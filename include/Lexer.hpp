#pragma once

#include <optional>

#include <string>

#include <string_view>

#include <unicode/uchar.h>
#include <unicode/unistr.h>

namespace seepp
{
    using Token = std::string_view;

    class Lexer
    {
        Token m_curr;

        Token chop(size_t n);

        template <typename Predicate>
        Token chopWhile(Predicate &&predicate);

    public: 
        explicit Lexer(const Token &content);
        
        std::optional<std::string> nextToken();
    };
}