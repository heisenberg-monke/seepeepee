#pragma once

#include <string>
#include <unordered_map>

#include <filesystem>

#include "Logger.hpp"
#include "TfIdf.hpp"

namespace seepp
{
    class FileSystem
    {
        Logger &m_logger;
        
    public:
        FileSystem();

        std::filesystem::path resolveDir(const std::filesystem::path &path) const;
        
        std::string loadXML(const std::filesystem::path &path) const;
        void loadXMLDir(const std::filesystem::path &dirPath, TermFreqIndex &tfIndex) const;

        void loadIndex(const std::filesystem::path &indexPath, TermFreqIndex &tfIndex) const;
        void checkIndex(const std::filesystem::path &indexPath) const;
        void saveIndex(const TermFreqIndex &tfIndex, const std::filesystem::path &indexPath) const;
    };
}