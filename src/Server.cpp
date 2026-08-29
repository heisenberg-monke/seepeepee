#include "Server.hpp"

#include <nlohmann/json.hpp>

namespace seepp
{
    Server::Server(const FileSystem &fs)
        : m_logger(Logger::getLogger()), m_fs(fs) {}

    void Server::init(const std::string &address, const Model *model) const
    {
        httplib::Server server;
        const int port = 8080;
        
        server.Get(R"(.*)", [&](const httplib::Request &req, httplib::Response &res) {
            serveRequest(model, req, res);
        });

        server.Post(R"(.*)", [&](const httplib::Request &req, httplib::Response &res) {
            serveRequest(model, req, res);
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

            std::error_code ec;
            const bool exists = std::filesystem::exists(filePath, ec);

            if(!exists && !ec)
                serve404(res);

            else
                serve500(res);
        }

        else
        {
            std::stringstream buffer;

            buffer << file.rdbuf();
            res.set_content(buffer.str(), contentType);
        }
    }

    void Server::serve404(httplib::Response &res) const
    {
        res.status = 404;
        res.set_content("404", "text/plain");
    }

    void Server::serve500(httplib::Response &res) const
    {
        res.status = 500;
        res.set_content("500", "text/plain");
    }

    void Server::serve400(httplib::Response &res, const std::string &message) const
    {
        res.status = 400;
        res.set_content("400: " + message, "text/plain");
    }

    void Server::serveAPISearch(const Model *model, const httplib::Request &req, httplib::Response &res) const
    {
        try
        {
            auto result = model->search(req.body);
            nlohmann::json json = nlohmann::json::array();

            for(size_t i = 0; i < std::min<size_t>(20, result.size()); ++i)
                json.push_back({result[i].first.string(), result[i].second});
                
            res.set_content(json.dump(), "application/json");
        }
        
        catch(const std::runtime_error &e) 
        {
            m_logger.err() << "Could not interpret body as a UTF-8 string.\n";
            serve400(res, "Body must be a valid UTF-8 string.\n");
        }

        catch(const nlohmann::json::exception &e)
        {
            m_logger.err() << "Could not convert search results to JSON: " << e.what() << '\n';
            serve500(res);
        }
    }

    void Server::serveRequest(const Model *model, const httplib::Request &req, httplib::Response &res) const
    {
        auto htmlPath = m_fs.resolveDir("index.html").string();
        auto jsPath = m_fs.resolveDir("index.js").string();

        m_logger.log() << "Received request! Method: " << req.method << ", URL: " << req.target << '\n';

        if(req.method == "POST" && req.target == "/api/search")
            serveAPISearch(model, req, res);

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