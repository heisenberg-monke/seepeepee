#pragma once

#include <string>

#include <iostream>

namespace seepp
{
    class Stream
    {
        std::ostream *m_stream;

    public:
        explicit Stream(std::ostream *stream)
            : m_stream(stream) {}

        template <typename T>
        Stream &operator<<(const T &value)
        {
            if(m_stream)
                *m_stream << value;

            return *this;
        }

        Stream& operator<<(std::ostream& (*manip)(std::ostream&))
        {
            if(m_stream)
                *m_stream << manip;

            return *this;
        }

    };

    class Logger
    {
        bool m_debug = false;
        Stream m_out;
        Stream m_log;
    
        Logger();
        Logger(const Logger &) = delete;
        Logger(Logger &&) = delete;

        Logger &operator=(const Logger &) = delete;
        Logger &operator=(Logger &&) = delete;

    public:
        static Logger &getLogger();

        void setDebug(bool debug);

        void log(const std::string &msg) const;
        void display(const std::string &msg) const;

        Stream &log() {
            return m_log;
        }

        Stream &display() {
            return m_out;
        }
    };
}