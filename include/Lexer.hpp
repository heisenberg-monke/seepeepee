#pragma once

#include <optional>

#include <vector>
#include <string_view>

namespace seepp
{
    using Token = std::string_view;

    class Lexer
    {
        Token m_curr;

        void trimLeft();

        Token chop(size_t n);

        template <typename Predicate>
        Token chopWhile(Predicate &&predicate);

    public: 
        explicit Lexer(const Token &content);

        const Token &getCurr() const
        {
            return m_curr;
        }
        
        std::optional<Token> nextToken();
    };
}