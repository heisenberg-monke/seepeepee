#include <fstream>
#include <iostream>

#include <algorithm>

#include <unordered_map>

#include "seepeepee.hpp"

#include <nlohmann/json.hpp>

enum class Subcommand
{
    NONE,
    HELP,
    INDEX,
    SEARCH,
    SERVE
};

int main(int argc, char **argv)
{
    try
    {
        if(argc < 2)
            throw std::runtime_error("No subcommand is given.");

        bool debug = false;
        Subcommand subcommand = Subcommand::NONE;
        
        std::filesystem::path indexPath;
        std::string jsonName;
        std::string address;

        auto setCommand = [&](Subcommand command)
        {
            if(subcommand != Subcommand::NONE)
                throw std::runtime_error("Only one subcommand can be specified.\n");

            subcommand = command;
        };

        for(int i = 1; i < argc; ++i)
        {
            std::string arg = argv[i];

            if(arg == "--debug" || arg == "-D")
                debug = true;

            else if(arg == "--help" || arg == "-H")
                setCommand(Subcommand::HELP);

            else if(arg == "--index" || arg == "-I")
            {
                setCommand(Subcommand::INDEX);

                if(i + 1 >= argc || argv[i+1][0] == '-')
                    throw std::runtime_error("No folder given for indexing.\n");

                indexPath = argv[++i];

                if(i + 1 < argc && argv[i+1][0] != '-')
                    jsonName = argv[++i];
            }

            else if(arg == "--search" || arg == "-S")
            {
                setCommand(Subcommand::SEARCH);

                if(i + 1 >= argc || argv[i+1][0] == '-')
                    throw std::runtime_error("No query given to search.\n");

                indexPath = argv[++i];
            }

            else if(arg == "--serve" || arg == "-s")
            {
                setCommand(Subcommand::SERVE);

                if(i + 1 >= argc || argv[i+1][0] == '-')
                    throw std::runtime_error("No file given for indexing.\n");

                indexPath = argv[++i];
                address = (i+1 < argc && argv[i+1][0] != '-') ? argv[++i] : "127.0.0.1";
            }

            else
                throw std::runtime_error("Unknown argument: " + arg);
        }

        if(subcommand == Subcommand::NONE)
            throw std::runtime_error("No subcommand is given.\n");

        auto &logger = seepp::Logger::getLogger();

        logger.setDebug(debug);

        try
        {
            seepp::App app;

            switch(subcommand)
            {
                case Subcommand::HELP:
                    app.showHelp();
                    break;

                case Subcommand::INDEX:
                    app.indexFolder(indexPath, jsonName);
                    break;

                case Subcommand::SEARCH:
                    app.search(indexPath);
                    break;

                case Subcommand::SERVE:
                    app.serve(indexPath, address);
                    break;
            }
        }

        catch(const std::exception &e)
        {
            logger.err() << e.what() << '\n';
            return EXIT_FAILURE;
        }
    }

    catch(const std::exception &e)
    {
        std::cerr << "[ERROR] " << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return 0;
}