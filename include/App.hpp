#pragma once

#include "FileSystem.hpp"
#include "Lexer.hpp"
#include "Logger.hpp"

namespace seepp
{
    class App
    {
        Logger &m_logger;
        FileSystem m_fs;

        std::string toUpper(const seepp::Token &token) const;

    public:
        App();
        
        void showHelp() const;
        void indexFolder(const std::filesystem::path &dirPath, const std::string &jsonName) const;
        void search(const std::filesystem::path &indexPath, const std::string &query) const;
        void serve(const std::filesystem::path &indexPath, const std::string &address) const;
    };
}
