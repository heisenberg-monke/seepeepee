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

    void App::serveRequest(const httplib::Request &req, httplib::Response &res) const
    {
        auto htmlPath = m_fs.resolveDir("index.html").string();
        auto jsPath = m_fs.resolveDir("index.js").string();

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
            m_logger.display() << "Search: " << req.body << '\n';
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
        m_logger.display("--help | -h                                     : Show this help box.");
        m_logger.display("--index <folder> [name] | -i <folder> [name]    : Index a folder and save it as a json.");
        m_logger.display("--search <query> | -S <query>                   : search for a query (not implemented yet).");
        m_logger.display("--serve | -s [address]                          : start local HTTP server with web interface");
    }

    void App::indexFolder(const std::filesystem::path &path, const std::string &jsonName) const
    {
        auto dirPath = m_fs.resolveDir(path);

        TermFreqIndex tfIndex;
        nlohmann::json j;

        for(const auto &[path, content] : m_fs.loadXMLDir(dirPath))
        {
            m_logger.log() << "Indexing " << path << "...";

            seepp::Lexer lexer(content);
            TermFreq tf;

            while(auto token = lexer.nextToken())
                tf[toUpper(token.value())]++;
            
            std::vector<std::pair<std::string, size_t>> stats(tf.begin(), tf.end());

            std::sort(stats.begin(), stats.end(), [](const auto &a, const auto &b) {
                return a.second > b.second;
            });

            tfIndex.try_emplace(path, tf);
        }
        
        std::string indexName = "index";

        if(!jsonName.empty())
        {
            indexName = indexName + "_" + jsonName;

            if(!indexName.ends_with(".json"))
                indexName += ".json";
        }
            
        auto indexPath = m_fs.resolveDir(indexName);

        m_logger.log() << "Saving " << indexPath << "...";

        for(const auto &[path, tf] : tfIndex)
            j[path.string()] = tf;

        std::ofstream out(indexPath);

        out << j.dump(2);
    }

    void App::search(const std::string &query) const
    {
        m_logger.display("Searching is not implemented yet");
    }

    void App::createServer(const std::string &address) const
    {
        httplib::Server server;
        const int port = 8080;
        

        server.Get(R"(.*)", [&](const httplib::Request &req, httplib::Response &res)
        {
            serveRequest(req, res);
        });

        server.Post(R"(.*)", [&](const httplib::Request &req, httplib::Response &res)
        {
            serveRequest(req, res);
        });

        m_logger.log() << "Listening at http://" << address << ":" << port << '\n';

        if(!server.listen(address, port))
            throw std::runtime_error("Could not start HTTP server");
    }
}