#include "FileSystem.hpp"
#include "Logger.hpp"
#include "Lexer.hpp"

#include <fstream>
#include <iostream>

#include "Model.hpp"
#include "pugixml.hpp"

#include <cerrno>
#include <cstring>

#include <istream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>

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

    std::string FileSystem::loadFile(const std::filesystem::path &path) const
    {
        auto ext = path.extension();

        if(ext == ".xtml" || ext == ".xml")
            return loadXML(path);

        else if(ext == ".txt" || ext == ".md")
            return loadTXT(path);

        throw std::runtime_error("Unsupported extension: " + ext.string());
    }

    void FileSystem::loadXMLDir(const std::filesystem::path &path, Model *model, size_t &skipped) const
    {
        auto dirPath = resolveDir(path);
        std::error_code ec;

        for(const auto &entry : std::filesystem::directory_iterator(dirPath, ec))
        {
            if(ec)
                throw std::runtime_error("Could not read directory: " + dirPath.string() + ": " + ec.message());

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
                loadXMLDir(filePath, model, skipped);
                continue;
            }

            m_logger.log() << "Indexing " << filePath << "...\n";

            std::string content;

            try
            {
                content = loadFile(filePath);
            }

            catch(const std::exception &e)
            {
                m_logger.err() << e.what() << '\n';
                ++skipped;
                continue;
            }

            model->addDocument(filePath, content);
        }

        if(ec)
            throw std::runtime_error("Could not iterate directory: " + dirPath.string() + ": " + ec.message());
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
            model.m_docs.try_emplace(std::filesystem::path(path), doc.at("tf").get<TermFreq>(), doc.at("size").get<size_t>());
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
                {"tf", doc.tf}
            };
        }

        out << j.dump(2);
    }
}