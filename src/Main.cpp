#include <iostream>

#include "seepeepee.hpp"

int main(int argc, char **argv)
{
    try
    {
        seepp::FileSystem fs;

        fs.loadXMLDir("docs.gl/gl4");
    }

    catch(const std::exception &e)
    {
        std::cerr << "[ERROR] " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    

    return 0;
}