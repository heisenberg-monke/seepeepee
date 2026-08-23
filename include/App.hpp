#pragma once

#include "FileSystem.hpp"
#include "Lexer.hpp"

namespace seepp
{
    using TermFreq = std::unordered_map<std::string, size_t>;
    using TermFreqIndex = std::unordered_map<std::filesystem::path, TermFreq>;

    class App
    {
        FileSystem m_fs;

        std::string toUpper(const seepp::Token &token) const;

    public:
        void showHelp() const;
        void indexFolder(const std::filesystem::path &path, const std::string &jsonName) const;
        void search(const std::string &query) const;
        void createServer(const std::string &address) const;
    };
}
