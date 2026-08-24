#include "App.hpp"

#include <algorithm>

#include <fstream>

#include <nlohmann/json.hpp>

namespace seepp
{
    std::string App::toUpper(const seepp::Token &token) const
    {
        std::string result(token);

        std::transform(result.begin(), result.end(), result.begin(), [](uint8_t c) {
            return std::toupper(c);
        });

        return result;
    }

    void App::serveRequest(const TermFreqIndex &tfIndex, const httplib::Request &req, httplib::Response &res) const
    {
        auto htmlPath = m_fs.resolveDir("index.html").string();
        auto jsPath = m_fs.resolveDir("index.js").string();

        TfIdf tfIdf;

        auto serveStaticFile = [&](const std::string &filePath, const std::string &contentType)
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
        };

        auto serve404 = [&]()
        {
            res.status = 404;
            res.set_content("404", "text/plain");
        };

        m_logger.log() << "Received request! Method: " << req.method << ", URL: " << req.target << '\n';

        if(req.method == "POST" && req.target == "/api/search")
        {
            std::vector<std::pair<std::filesystem::path, double>> result;

            for(const auto &[path, tf] : tfIndex)
            {
                Lexer lexer(req.body);
                double totalTf = 0.0;

                while(auto token = lexer.nextToken())
                    totalTf += tfIdf.termFreq(token.value(), tf);

                result.emplace_back(path, totalTf);
            }

            std::sort(result.begin(), result.end(), [](const auto &a, const auto &b) 
            {
                return a.second > b.second;
            });

            for(const auto &[path, rank] : result)
                m_logger.log() << path << " => " << rank << '\n';
            
            res.set_content("ok", "text/plain");
        }

        else if(req.method == "GET")
        {
            if(req.target == "/" || req.target == "/index.html")
                serveStaticFile(htmlPath, "text/html; charset=utf-8");

            else if(req.target == "/index.js")
                serveStaticFile(jsPath, "text/javascript; charset=utf-8");

            else
                serve404();
        }
        
        else
            serve404();
    }

    App::App()
        : m_logger(Logger::getLogger()) {}

    void App::showHelp() const
    {
        m_logger.display("Usage: {program} [SUBCOMMAND] [OPTIONS]");
        m_logger.display("Available commands: ");
        m_logger.display("--help | -H                                                   : Show this help box.");
        m_logger.display("--index <folder> [name] | -I <folder> [name]                  : Index a folder and save it as a json.");
        m_logger.display("--search <query> | -S <query>                                 : search for a query (not implemented yet).");
        m_logger.display("--serve <index-file> [address] | -s <index-file> [address]    : start local HTTP server with web interface");
    }

    void App::indexFolder(const std::filesystem::path &dirPath, const std::string &jsonName) const
    {
        TermFreqIndex tfIndex;

        m_fs.loadXMLDir(dirPath, tfIndex);
        m_fs.saveIndex(tfIndex, jsonName);
    }

    void App::search(const std::filesystem::path &indexPath) const
    {
        m_fs.checkIndex(indexPath);
    }

    void App::serve(const std::filesystem::path &indexPath, const std::string &address) const
    {
        TermFreqIndex tfIndex;
        
        m_fs.loadIndex(indexPath, tfIndex);

        httplib::Server server;
        const int port = 8080;
        
        server.Get(R"(.*)", [&](const httplib::Request &req, httplib::Response &res)
        {
            serveRequest(tfIndex, req, res);
        });

        server.Post(R"(.*)", [&](const httplib::Request &req, httplib::Response &res)
        {
            serveRequest(tfIndex, req, res);
        });

        m_logger.log() << "Listening at http://" << address << ":" << port << '\n';

        if(!server.listen(address, port))
            throw std::runtime_error("Could not start HTTP server");
    }
}