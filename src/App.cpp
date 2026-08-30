#include "App.hpp"
#include "Model.hpp"
#include "Server.hpp"
#include <memory>

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

    App::App(bool sqlite)
        : m_logger(Logger::getLogger()), m_sqlite(sqlite) {}

    void App::showHelp() const
    {
        m_logger.display("Usage: {program} [SUBCOMMAND] [OPTIONS]\n");
        
        m_logger.display("Available flags: ");
        m_logger.display("--debug | -D      : Show debug logs on the terminal.");
        m_logger.display("--sqlite | -Sq    : Use SQLite instead of JSON.\n");
        
        m_logger.display("Available commands: ");
        m_logger.display("--help | -H                                                   : Show this help box.");
        m_logger.display("--index <folder> [name] | -I <folder> [name]                  : Index a folder and save it as a json.");
        m_logger.display("--search <index-file> <query> | -S <index-file> <query>       : search for a query within the index file.");
        m_logger.display("--serve <index-file> [address] | -s <index-file> [address]    : start local HTTP server with web interface");
    }

    void App::indexFolder(const std::filesystem::path &dirPath, const std::string &jsonName) const
    {
        size_t skipped = 0;

        if(!m_sqlite)
        {
            InMemoryModel model;

            skipped = m_fs.loadDir(dirPath, &model);
            m_fs.saveModel(model, jsonName);
        }
        
        else
        {
            SQLiteModel model;
            

            m_fs.loadModel(jsonName, model);
            model.begin();
            skipped = m_fs.loadDir(dirPath, &model);
            model.commit();
        }

        m_logger.log() << "Skipped " << skipped << " files.\n";
    }

    void App::search(const std::filesystem::path &modelPath, const std::string &query) const
    {
        std::unique_ptr<Model> model;
        
        if(m_sqlite) 
            model = std::make_unique<SQLiteModel>();
        else 
            model = std::make_unique<InMemoryModel>();

        m_fs.loadModel(modelPath, model.get());

        auto result = model->search(query);

        for(size_t i = 0; i < std::min<size_t>(20, result.size()); ++i)
            m_logger.display() << result[i].first << " => " << result[i].second << '\n';
    }

    void App::serve(const std::filesystem::path &modelPath, const std::string &address) const
    {
        Server server(m_fs);
        std::unique_ptr<Model> model;
        
        if(m_sqlite) 
            model = std::make_unique<SQLiteModel>();
        else 
            model = std::make_unique<InMemoryModel>();

        m_fs.loadModel(modelPath, model.get());
        server.init(address, model.get());
    }
}