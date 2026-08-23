#include "Logger.hpp"

#include <iostream>

namespace seepp
{
    Logger::Logger()
        : m_out(&std::cout), m_log(nullptr), m_err(&std::cerr){}

    Logger &Logger::getLogger()
    {
        static Logger logger;
        return logger;
    }

    void Logger::setDebug(bool debug)
    {
        m_debug = debug;
        m_log = Stream(debug ? &std::clog : nullptr);
    }

    void Logger::log(const std::string &msg) const
    {
        if(m_debug)
            std::clog << msg << '\n';
    }

    void Logger::display(const std::string &msg) const
    {
        std::cout << msg << '\n';
    }
}