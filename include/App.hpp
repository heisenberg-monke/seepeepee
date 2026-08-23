#pragma once

#include "FileSystem.hpp"
#include "Lexer.hpp"
#include "Logger.hpp"

#include <cpp-httplib/httplib.h>

namespace seepp
{
    using TermFreq = std::unordered_map<std::string, size_t>;
    using TermFreqIndex = std::unordered_map<std::filesystem::path, TermFreq>;

    class App
    {
        Logger &m_logger;
        FileSystem m_fs;

        std::string toUpper(const seepp::Token &token) const;
        void serveRequest(const httplib::Request &req, httplib::Response &res) const;

    public:
        App();
        void showHelp() const;
        void indexFolder(const std::filesystem::path &path, const std::string &jsonName) const;
        void search(const std::string &query) const;
        void createServer(const std::string &address) const;
    };
}
