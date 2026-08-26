#pragma once

#include <string>
#include <unordered_map>

#include <filesystem>

#include "Logger.hpp"
#include "Model.hpp"

namespace seepp
{
    class FileSystem
    {
        Logger &m_logger;
        
    public:
        FileSystem();

        std::filesystem::path resolveDir(const std::filesystem::path &path) const;
        
        std::string loadXML(const std::filesystem::path &path) const;
        void loadXMLDir(const std::filesystem::path &dirPath, TermFreqPerDoc &tfIndex) const;

        void loadModel(const std::filesystem::path &modelPath, Model &model) const;
        // void checkIndex(const std::filesystem::path &indexPath) const;
        void saveModel(const Model &model, const std::filesystem::path &modelPath) const;
    };
}