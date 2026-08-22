#include "FileSystem.hpp"

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

    std::string FileSystem::loadXML(const std::filesystem::path &path) const
    {
        auto filePathStr = path.string();

        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load_file(path.c_str());

        if(!result)
            throw std::runtime_error(std::string("Failed to open file: ") + filePathStr);

        std::string content;

        extractVisibleText(doc, content);

        return content;
    }

    void FileSystem::loadXMLDir(const std::filesystem::path &path) const
    {
        auto dirPath = std::filesystem::path(PROJECT_ROOT) / path;

        if(!std::filesystem::exists(dirPath) || !std::filesystem::is_directory(dirPath))
            throw std::runtime_error("Path " + dirPath.string() + " is not a valid directory.");

        for(const auto &entry : std::filesystem::directory_iterator(dirPath))
        {
            if(entry.is_regular_file() && entry.path().extension() == ".xhtml")
                std::cout << "Entry " << entry.path().string() << " has length " << loadXML(entry).length() << '\n';
        }
    }
}