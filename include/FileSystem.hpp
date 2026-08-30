#pragma once

#include <string>

#include <filesystem>

#include "Logger.hpp"
#include "Model.hpp"

namespace seepp
{
    class FileSystem
    {
        Logger &m_logger;

        void collectFiles(const std::filesystem::path &dir, std::vector<std::filesystem::path> &files) const;
        
    public:
        FileSystem();

        std::filesystem::path resolveDir(const std::filesystem::path &path) const;
        
        std::string loadXML(const std::filesystem::path &path) const;
        std::string loadTXT(const std::filesystem::path &path) const;
        std::string loadPDF(const std::filesystem::path &path) const;
        std::string loadFile(const std::filesystem::path &path) const;
        
        size_t loadDir(const std::filesystem::path &dirPath, Model *model) const;

        void loadModel(const std::filesystem::path &modelPath, InMemoryModel &model) const;
        void loadModel(const std::filesystem::path &modelPath, SQLiteModel &model) const;
        void loadModel(const std::filesystem::path &modelPath, Model *model) const;

        // void checkIndex(const std::filesystem::path &indexPath) const;
        
        void saveModel(const InMemoryModel &model, const std::filesystem::path &modelPath) const;
    };
}