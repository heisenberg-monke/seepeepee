#pragma once

#include <string>

#include <filesystem>

#include "pugixml.hpp"

namespace seepp
{
    class FileSystem
    {
        void extractVisibleText(const pugi::xml_node &node, std::string &output) const;

    public:
        std::string loadXML(const std::filesystem::path &path) const;
        void loadXMLDir(const std::filesystem::path &path) const;
    };
}