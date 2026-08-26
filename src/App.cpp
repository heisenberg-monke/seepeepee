#include "App.hpp"
#include "Server.hpp"

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

    App::App()
        : m_logger(Logger::getLogger()) {}

    void App::showHelp() const
    {
        m_logger.display("Usage: {program} [SUBCOMMAND] [OPTIONS]");
        m_logger.display("Available commands: ");
        m_logger.display("--help | -H                                                   : Show this help box.");
        m_logger.display("--index <folder> [name] | -I <folder> [name]                  : Index a folder and save it as a json.");
        m_logger.display("--search <index-file> <query> | -S <index-file> <query>       : search for a query within the index file.");
        m_logger.display("--serve <index-file> [address] | -s <index-file> [address]    : start local HTTP server with web interface");
    }

    void App::indexFolder(const std::filesystem::path &dirPath, const std::string &jsonName) const
    {
        TermFreqIndex tfIndex;

        m_fs.loadXMLDir(dirPath, tfIndex);
        m_fs.saveIndex(tfIndex, jsonName);
    }

    void App::search(const std::filesystem::path &indexPath, const std::string &query) const
    {
        TermFreqIndex tfIndex;
        Model model;
        
        m_fs.loadIndex(indexPath, tfIndex);

        auto result = model.search(tfIndex, query);

        for(size_t i = 0; i < std::min<size_t>(20, result.size()); ++i)
            m_logger.display() << *result[i].first << " => " << result[i].second << '\n';
    }

    void App::serve(const std::filesystem::path &indexPath, const std::string &address) const
    {
        Server server(m_fs);
        TermFreqIndex tfIndex;

        m_fs.loadIndex(indexPath, tfIndex);
        server.init(address, tfIndex);
    }
}