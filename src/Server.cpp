#include "Server.hpp"

#include <nlohmann/json.hpp>

namespace seepp
{
    Server::Server(const FileSystem &fs)
        : m_logger(Logger::getLogger()), m_fs(fs) {}

    void Server::init(const std::string &address, TermFreqIndex &tfIndex) const
    {
        httplib::Server server;
        const int port = 8080;
        
        server.Get(R"(.*)", [&](const httplib::Request &req, httplib::Response &res) {
            serveRequest(tfIndex, req, res);
        });

        server.Post(R"(.*)", [&](const httplib::Request &req, httplib::Response &res) {
            serveRequest(tfIndex, req, res);
        });

        m_logger.log() << "Listening at http://" << address << ":" << port << '\n';

        if(!server.listen(address, port))
            throw std::runtime_error("Could not start HTTP server");
    }

    void Server::serveStaticFile(httplib::Response &res, const std::string &filePath, const std::string &contentType) const
    {
        std::ifstream file(filePath);

        if(!file)
        {
            m_logger.err() << "Could not serve file " << filePath << '\n';

            res.status = 500;
            res.set_content("500", "text/plain");

            return;
        }

        std::stringstream buffer;

        buffer << file.rdbuf();
        res.set_content(buffer.str(), contentType);
    }

    void Server::serve404(httplib::Response &res) const
    {
        res.status = 404;
        res.set_content("404", "text/plain");
    }

    void Server::serveAPISearch(TermFreqIndex &tfIndex, const httplib::Request &req, httplib::Response &res) const
    {
        Model model;

        auto result = model.search(tfIndex, req.body);
        nlohmann::json json = nlohmann::json::array();

        for(size_t i = 0; i < std::min<size_t>(20, result.size()); ++i)
            json.push_back({result[i].first->string(), result[i].second});
            
        res.set_content(json.dump(), "application/json");
    }

    void Server::serveRequest(TermFreqIndex &tfIndex, const httplib::Request &req, httplib::Response &res) const
    {
        auto htmlPath = m_fs.resolveDir("index.html").string();
        auto jsPath = m_fs.resolveDir("index.js").string();

        m_logger.log() << "Received request! Method: " << req.method << ", URL: " << req.target << '\n';

        if(req.method == "POST" && req.target == "/api/search")
            serveAPISearch(tfIndex, req, res);

        else if(req.method == "GET")
        {
            if(req.target == "/" || req.target == "/index.html")
                serveStaticFile(res, htmlPath, "text/html; charset=utf-8");

            else if(req.target == "/index.js")
                serveStaticFile(res, jsPath, "text/javascript; charset=utf-8");

            else
                serve404(res);
        }
        
        else
            serve404(res);
    }
}