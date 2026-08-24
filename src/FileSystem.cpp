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

    void FileSystem::loadXMLDir(const std::filesystem::path &path, TermFreqIndex &tfIndex) const
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
                loadXMLDir(filePath, tfIndex);
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

            tfIndex[filePath] = std::move(tf);
        }

        if(ec)
            throw std::runtime_error("Could not iterate directory: " + dirPath.string() + ": " + ec.message());
    }

    void FileSystem::loadIndex(const std::filesystem::path &path, TermFreqIndex &tfIndex) const
    {
        auto indexPath = resolveDir(path);

        m_logger.log() << "Reading " << indexPath << "...\n";

        std::ifstream indexFile(indexPath);
        nlohmann::json j;

        if(!indexFile.is_open())
            throw std::runtime_error("Could not open index file: " + indexPath.string() + ": " + std::strerror(errno));

        indexFile >> j;

        for(const auto &[path, tf] : j.items())
            tfIndex.try_emplace(std::filesystem::path(path), tf.get<TermFreq>());
    }

    void FileSystem::checkIndex(const std::filesystem::path &path) const
    {
        auto indexPath = resolveDir(path);
        TermFreqIndex tfIndex;

        loadIndex(indexPath, tfIndex);
        m_logger.display() << indexPath << " contains " << tfIndex.size() << " files.\n";
    }

    void FileSystem::saveIndex(const TermFreqIndex &tfIndex, const std::filesystem::path &indexPath) const
    {
        nlohmann::json j;
        std::ofstream out(indexPath);

        if(!out.is_open())
            throw std::runtime_error("Could not create index file " + indexPath.string() + ": " + std::strerror(errno));

        m_logger.log() << "Saving " << indexPath << "...";

        for(const auto &[path, tf] : tfIndex)
            j[path.string()] = tf;

        out << j.dump(2);
    }
}