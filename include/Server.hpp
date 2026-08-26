#pragma once

#include <cpp-httplib/httplib.h>

#include <string>

#include "Logger.hpp"
#include "Model.hpp"
#include "FileSystem.hpp"

namespace seepp
{
    class Server
    {
        Logger &m_logger;
        const FileSystem &m_fs;

    public:
        Server(const FileSystem &fs);

        void init(const std::string &address, TermFreqIndex &tfIndex) const;

        void serveStaticFile(httplib::Response &res, const std::string &filePath, const std::string &contentType) const;
        void serve404(httplib::Response &res) const;
        void serveAPISearch(TermFreqIndex &tfIndex, const httplib::Request &req, httplib::Response &res) const;
        void serveRequest(TermFreqIndex &tfIndex, const httplib::Request &req, httplib::Response &res) const;
    };
}