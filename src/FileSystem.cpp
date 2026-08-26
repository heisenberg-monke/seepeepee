#include "FileSystem.hpp"
#include "Logger.hpp"
#include "Lexer.hpp"

#include <fstream>
#include <iostream>

#include "pugixml.hpp"

#include <cerrno>
#include <cstring>

#include <nlohmann/json.hpp>

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

    void FileSystem::loadXMLDir(const std::filesystem::path &path, Model &model) const
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
                loadXMLDir(filePath, model);
                continue;
            }

            m_logger.log() << "Indexing " << filePath << "...\n";

            std::string content;

            try
            {
                content = loadXML(filePath);
            }

            catch(const std::exception &e)
            {
                m_logger.err() << e.what() << '\n';
                continue;
            }

            TermFreq tf;
            Lexer lexer(content);

            while(auto token = lexer.nextToken())
                tf[token.value()]++;

            for(const auto &[term, _] : tf)
                model.m_df[term]++;

            model.m_tfpd[filePath] = std::move(tf);
        }

        if(ec)
            throw std::runtime_error("Could not iterate directory: " + dirPath.string() + ": " + ec.message());
    }

    void FileSystem::loadModel(const std::filesystem::path &path, Model &model) const
    {
        auto modelPath = resolveDir(path);

        m_logger.log() << "Reading " << modelPath << "...\n";

        std::ifstream modelFile(modelPath);
        nlohmann::json j;

        if(!modelFile)
            throw std::runtime_error("Could not open index file: " + modelPath.string() + ": " + std::strerror(errno));

        modelFile >> j;

        model.m_df = j.at("df").get<DocFreq>();

        model.m_tfpd.clear();

        for(const auto &[path, tf] : j.at("tfpd").items())
            model.m_tfpd.try_emplace(std::filesystem::path(path), tf.get<TermFreq>());
    }

    // void FileSystem::checkIndex(const std::filesystem::path &path) const
    // {
    //     auto indexPath = resolveDir(path);
    //     TermFreqPerDoc tfIndex;

    //     loadIndex(indexPath, tfIndex);
    //     m_logger.display() << indexPath << " contains " << tfIndex.size() << " files.\n";
    // }

    void FileSystem::saveModel(const Model &model, const std::filesystem::path &modelPath) const
    {
        nlohmann::json j;
        std::ofstream out(modelPath);

        if(!out)
            throw std::runtime_error("Could not create index file " + modelPath.string() + ": " + std::strerror(errno));

        m_logger.log() << "Saving " << modelPath << "...";

        j["df"] = model.m_df;

        for(const auto &[path, tf] : model.m_tfpd)
            j["tfpd"][path.string()] = tf;

        out << j.dump(2);
    }
}