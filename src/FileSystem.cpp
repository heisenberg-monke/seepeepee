#include "FileSystem.hpp"
#include "Logger.hpp"

#include <iostream>

namespace seepp
{
    void FileSystem::extractVisibleText(const pugi::xml_node &node, std::string &output) const
    {
        for(pugi::xml_node child : node.children())
        {
            if(child.type() == pugi::node_pcdata)
            {
                output += child.value();
                continue;
            }

            if(child.type() != pugi::node_element)
                continue;

            const std::string name = child.name();

            if(name == "script" || name == "style" || name == "head" || name == "noscript" || name == "template")
                continue;

            output += ' ';

            extractVisibleText(child, output);
        }
    }

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

    std::unordered_map<std::filesystem::path, std::string> FileSystem::loadXMLDir(const std::filesystem::path &path) const
    {
        auto dirPath = resolveDir(path);
        auto &logger = Logger::getLogger();

        if(!std::filesystem::exists(dirPath) || !std::filesystem::is_directory(dirPath))
            throw std::runtime_error("Path " + dirPath.string() + " is not a valid directory.");

        std::unordered_map<std::filesystem::path, std::string> files;

        for(const auto &entry : std::filesystem::directory_iterator(dirPath))
        {
            if(entry.is_regular_file() && entry.path().extension() == ".xhtml")
                files[entry.path()] = loadXML(entry.path());

            else
                logger.log() << "Skipping non-XHTML file " << entry.path();
        }

        return files;
    }
}