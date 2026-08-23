#include <fstream>
#include <iostream>

#include <algorithm>

#include <unordered_map>

#include "seepeepee.hpp"

#include <nlohmann/json.hpp>

int main(int argc, char **argv)
{
    try
    {
        if(argc < 2)
            throw std::runtime_error("No subcommand is given.");

        bool debug = false;
        bool help = false;
        std::filesystem::path indexPath;
        std::string query;
        std::string jsonName;
        std::string address;

        for(int i = 1; i < argc; ++i)
        {
            std::string arg = argv[i];

            if(arg == "--debug" || arg == "-D")
                debug = true;

            if(arg == "--index" || arg == "-I")
            {
                if(i + 1 >= argc)
                    throw std::runtime_error("No folder given for indexing.\n");

                if(i + 2 < argc && argv[i+2][0] != '-')
                    jsonName = argv[i+2];

                indexPath = argv[i+1];
            }

            if(arg == "--search" || arg == "-S")
            {
                if(i + 1 >= argc)
                    throw std::runtime_error("No query given to search.\n");

                query = argv[i+1];
            }

            if(arg == "--help" || arg == "-H")
                help = true;

            if(arg == "--serve" || arg == "-s")
                address = (i+1 < argc && argv[i+1][0] != '-') ? argv[i+1] : "127.0.0.1";
        }

        auto &logger = seepp::Logger::getLogger();

        logger.setDebug(debug);

        seepp::App app;

        if(help)
            app.showHelp();

        else if(!address.empty())
            app.createServer(address);

        else if(!indexPath.empty())
            app.indexFolder(indexPath, jsonName);

        else if(!query.empty())
            app.search(query);
    }

    catch(const std::exception &e)
    {
        std::cerr << "[ERROR] " << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return 0;
}