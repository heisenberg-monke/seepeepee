#include "App.hpp"
#include "Logger.hpp"

#include <algorithm>

#include <fstream>

#include <nlohmann/json.hpp>
#include <cpp-httplib/httplib.h>

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

    void App::showHelp() const
    {
        auto &logger = Logger::getLogger();

        logger.display("Usage: {program} [SUBCOMMAND] [OPTIONS]");
        logger.display("Available commands: ");
        logger.display("--help | -h                                     : Show this help box.");
        logger.display("--index <folder> [name] | -i <folder> [name]    : Index a folder and save it as a json.");
        logger.display("--search <query> | -S <query>                   : search for a query (not implemented yet).");
        logger.display("--serve | -s [address]                          : start local HTTP server with web interface");
    }

    void App::indexFolder(const std::filesystem::path &path, const std::string &jsonName) const
    {
        auto &logger = Logger::getLogger();
        auto dirPath = m_fs.resolveDir(path);

        TermFreqIndex tfIndex;
        nlohmann::json j;

        for(const auto &[path, content] : m_fs.loadXMLDir(dirPath))
        {
            logger.log("Indexing " + path.string() + "...");

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

        logger.log("Saving " + indexPath.string() + "...");

        for(const auto &[path, tf] : tfIndex)
            j[path.string()] = tf;

        std::ofstream out(indexPath);

        out << j.dump(2);
    }

    void App::search(const std::string &query) const
    {
        Logger::getLogger().display("Searching is not implemented yet");
    }

    void App::createServer(const std::string &address) const
    {
        auto &logger = Logger::getLogger();

        httplib::Server server;
        const int port = 8080;
        auto path = m_fs.resolveDir("index.html").string();

        server.set_pre_routing_handler([&](const httplib::Request &req, httplib::Response &res)
        {
            logger.log() << "[INFO] Received request! Method: " << req.method << ", URL: " << req.target << '\n';
            res.set_file_content(path, "text/html");

            return httplib::Server::HandlerResponse::Handled;
        });

        logger.log() << "[INFO] Listening at http://" << address << ":" << port << '\n';

        if(!server.listen(address, port))
            throw std::runtime_error("Could not start HTTP server");
    }
}