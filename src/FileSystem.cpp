#include "FileSystem.hpp"
#include "Logger.hpp"

#include <exception>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>

#include "Model.hpp"
#include "pugixml.hpp"

#include <cerrno>
#include <cstring>
#include <memory>
#include <string>
#include <thread>

#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <unordered_set>
#include <vector>

#include "poppler-document.h"
#include "poppler-page.h"

namespace seepp
{
    static inline void extractVisibleText(const pugi::xml_node &node, std::string &output)
    {
        for(pugi::xml_node child : node.children())
        {
            if(child.type() == pugi::node_pcdata || child.type() == pugi::node_cdata)
            {
                output += child.value();
                output += ' ';
            }

            if(child.type() == pugi::node_element)
                extractVisibleText(child, output);
        }
    }

    static inline bool isUnderDir(const std::filesystem::path &file, const std::filesystem::path &dir)
    {
        auto relative = file.lexically_relative(dir);

        if(relative.empty())
            return false;

        auto it = relative.begin();

        if(it == relative.end())
            return false;

        return *it != "..";
    }

    void FileSystem::collectFiles(const std::filesystem::path &dir, std::vector<std::filesystem::path> &files) const
    {
        std::error_code ec;

        for(const auto &entry : std::filesystem::directory_iterator(dir, ec))
        {
            if(ec)
                throw std::runtime_error("Could not read directory: " + dir.string() + ": " + ec.message());

            std::error_code typeEc;
            
            const auto &filePath = entry.path();
            const auto status = entry.symlink_status(typeEc);

            if(typeEc)
            {
                m_logger.err() << "Could not determine the type of file: " << filePath << ": " << typeEc.message() << '\n';
                continue;
            }

            if(std::filesystem::is_directory(status))
            {
                collectFiles(filePath, files);
                continue;
            }

            files.push_back(filePath);
        }

        if(ec)
            throw std::runtime_error("Could not iterate directory: " + dir.string() + ": " + ec.message());
    }

    FileSystem::FileSystem()
        : m_logger(Logger::getLogger()) {}

    std::filesystem::path FileSystem::resolveDir(const std::filesystem::path &path) const
    {
        if(!path.is_absolute())
            return PROJECT_ROOT / path;

        return path;
    }

    std::string FileSystem::loadXML(const std::filesystem::path &path) const
    {
        auto filePath = resolveDir(path);
        auto filePathStr = filePath.string();

        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load_file(filePath.c_str());

        if(!result)
            throw std::runtime_error(std::string("Failed to open file: ") + filePathStr);

        std::string content;

        extractVisibleText(doc, content);

        return content;
    }

    std::string FileSystem::loadTXT(const std::filesystem::path &path) const
    {
        auto filePath = resolveDir(path);

        std::ifstream file(filePath);
        std::ostringstream buffer;

        if(!file)
            throw std::runtime_error("Failed to open file: " + filePath.string());

        buffer << file.rdbuf();

        return buffer.str();
    }

    std::string FileSystem::loadPDF(const std::filesystem::path &path) const
    {
        auto filePath = path.string();
        std::unique_ptr<poppler::document> document(poppler::document::load_from_file(filePath));

        if(!document)
            throw std::runtime_error("Could not open PDF: " + filePath);

        if(document->is_locked())
            throw std::runtime_error("PDF is password-protected: " + filePath);

        std::string text;
        const int pageCount = document->pages();

        for(int pgNo = 0; pgNo < pageCount; ++pgNo)
        {
            std::unique_ptr<poppler::page> page(document->create_page(pgNo));

            if(!page)
                continue;

            const auto pageText = page->text().to_utf8();

            text.append(pageText.data(), pageText.size());
            text += '\n';
        }

        return text;
    }

    std::string FileSystem::loadFile(const std::filesystem::path &path) const
    {
        auto ext = path.extension();

        if(ext == ".xhtml" || ext == ".xml")
            return loadXML(path);

        else if(ext == ".txt" || ext == ".md" || ext == ".html")
            return loadTXT(path);

        else if(ext == ".pdf")
            return loadPDF(path);

        throw std::runtime_error("Unsupported extension: " + ext.string());
    }

    size_t FileSystem::loadDir(const std::filesystem::path &path, Model *model) const
    {
        size_t skipped = 0;
        const auto dirPath = resolveDir(path);

        std::vector<std::filesystem::path> files;
        collectFiles(dirPath, files);

        std::unordered_set<std::filesystem::path> curr;

        for(const auto &filePath : files)
            curr.insert(filePath);

        for(const auto &indexed : model->documents())
        {
            if(!isUnderDir(indexed, dirPath))
                continue;

            if(curr.contains(indexed))
                continue;

            m_logger.log() << "Removing " << indexed << " from index... \n";
            model->removeDocument(indexed);
        }

        const uint workerSize = std::max<uint>(1, std::thread::hardware_concurrency());

        for(size_t start = 0; start < files.size(); start += workerSize)
        {
            const size_t end = std::min(files.size(), start + workerSize);
            std::vector<std::future<bool>> workers;

            workers.reserve(end - start);

            for(size_t i = start; i < end; ++i)
            {
                workers.emplace_back(std::async(std::launch::async, [this, model, filePath = files[i]]() -> bool 
                {
                    try {
                        std::error_code timeEc;
                        const auto lastModified = std::filesystem::last_write_time(filePath, timeEc);

                        if(timeEc)
                        {
                            m_logger.err() << "Could not determine the last modification time for " << filePath << ": " << timeEc.message() << '\n';
                            return false;
                        }

                        if(!model->needsIndexing(filePath, lastModified))
                        {
                            m_logger.log() << "Skipping " << filePath << " (unchanged)\n";
                            return true;
                        }

                        m_logger.log() << "Indexing " << filePath << "...\n";
                        model->addDocument(filePath, loadFile(filePath), lastModified);

                        return true;
                    } catch(const std::exception &e) 
                    {
                        m_logger.err() << e.what() << '\n';
                        return false;
                    }
                }));
            }

            for(auto &worker : workers)
            {
                if(!worker.get())
                    ++skipped;
            }
        }

        return skipped;
    }

    void FileSystem::loadModel(const std::filesystem::path &path, InMemoryModel &model) const
    {
        auto modelPath = resolveDir(path);

        m_logger.log() << "Reading " << modelPath << "...\n";

        std::ifstream modelFile(modelPath);
        nlohmann::json j;

        if(!modelFile)
            throw std::runtime_error("Could not open index file: " + modelPath.string() + ": " + std::strerror(errno));

        modelFile >> j;

        model.m_df = j.at("df").get<DocFreq>();

        model.m_docs.clear();

        for(const auto &[path, doc] : j.at("tfpd").items())
            model.m_docs.try_emplace(std::filesystem::path(path), doc.at("tf").get<TermFreq>(), doc.at("size").get<size_t>(), std::filesystem::file_time_type(std::filesystem::file_time_type::duration(doc.at("lastModified").get<long long>())));
    }

    void FileSystem::loadModel(const std::filesystem::path &path, SQLiteModel &model) const
    {
        auto modelPath = resolveDir(path);
        sqlite3 *raw = nullptr;

        if(sqlite3_open(modelPath.c_str(), &raw) != SQLITE_OK)
        {
            std::string error = sqlite3_errmsg(raw);

            sqlite3_close(raw);
            throw std::runtime_error("Could not open SQLite database: " + modelPath.string() + ": " + error + '\n');
        }

        model.m_connection.reset(raw);

        if(!model.execute(
        R"sql(
            CREATE TABLE IF NOT EXISTS Documents
            (
                id INTEGER NOT NULL PRIMARY KEY,
                path TEXT,
                term_count INTEGER,
                last_modified INTEGER,
                
                UNIQUE(path)
            );
        )sql"))
            throw std::runtime_error("Failed to create table: Documents");

        if(!model.execute(
        R"sql(
            CREATE TABLE IF NOT EXISTS TermFreq
            (
                term TEXT,
                doc_id INTEGER,
                freq INTEGER,
                
                UNIQUE(term, doc_id),
                FOREIGN KEY(doc_id) REFERENCES Documents(id)
            );
        )sql"))
            throw std::runtime_error("Failed to create table: TermFreq");

        if(!model.execute(
        R"sql(
            CREATE TABLE IF NOT EXISTS DocFreq
            (
                term TEXT,
                freq INTEGER,
                
                UNIQUE(term)
            );
        )sql"))
            throw std::runtime_error("Failed to create table: DocFreq");
    }

    void FileSystem::loadModel(const std::filesystem::path &modelPath, Model *model) const
    {
        if(auto a = dynamic_cast<InMemoryModel *>(model))
            loadModel(modelPath, *a);

        else if(auto b = dynamic_cast<SQLiteModel *>(model))
            loadModel(modelPath, *b);
    }

    // void FileSystem::checkIndex(const std::filesystem::path &path) const
    // {
    //     auto indexPath = resolveDir(path);
    //     TermFreqPerDoc tfIndex;

    //     loadIndex(indexPath, tfIndex);
    //     m_logger.display() << indexPath << " contains " << tfIndex.size() << " files.\n";
    // }

    void FileSystem::saveModel(const InMemoryModel &model, const std::filesystem::path &modelPath) const
    {
        nlohmann::json j;
        std::ofstream out(modelPath);

        if(!out)
            throw std::runtime_error("Could not create index file " + modelPath.string() + ": " + std::strerror(errno));

        m_logger.log() << "Saving " << modelPath << "...";

        j["df"] = model.m_df;

        for(const auto &[path, doc] : model.m_docs)
        {
            j["tfpd"][path.string()] =
            {
                {"size", doc.count},
                {"tf", doc.tf},
                {"lastModified", doc.lastModified.time_since_epoch().count()}
            };
        }

        out << j.dump(2);
    }
}