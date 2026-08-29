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

        void init(const std::string &address, const Model *model) const;

        void serveStaticFile(httplib::Response &res, const std::string &filePath, const std::string &contentType) const;
        void serve404(httplib::Response &res) const;
        void serve500(httplib::Response &res) const;
        void serve400(httplib::Response &res, const std::string &message) const;
        void serveAPISearch(const Model *model, const httplib::Request &req, httplib::Response &res) const;
        void serveRequest(const Model *model, const httplib::Request &req, httplib::Response &res) const;
    };
}