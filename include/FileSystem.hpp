#pragma once

#include <string>
#include <unordered_map>

#include <filesystem>

#include "pugixml.hpp"

namespace seepp
{
    class FileSystem
    {
        void extractVisibleText(const pugi::xml_node &node, std::string &output) const;

    public:
        std::filesystem::path resolveDir(const std::filesystem::path &path) const;
        
        std::string loadXML(const std::filesystem::path &path) const;
        std::unordered_map<std::filesystem::path, std::string> loadXMLDir(const std::filesystem::path &path) const;
    };
}