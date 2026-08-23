#include "Lexer.hpp"

#include <cctype>

#include "utfcpp/utf8.h"

namespace seepp
{
    char32_t peek(const Token &token, size_t &bytes)
    {
        auto it = token.begin();
        char32_t c = utf8::next(it, token.end());

        bytes = it - token.begin();
        return c;
    }

    bool isSpace(char32_t c)
    {
        return c == U' ' ||
               c == U'\t' ||
               c == U'\n' ||
               c == U'\r' ||
               c == U'\f' ||
               c == U'\v';
    }

    bool isDigit(char32_t c)
    {
        return c >= U'0' && c <= U'9';
    }

    bool isAlpha(char32_t c)
    {
        return (c >= U'a' && c <= U'z') ||
               (c >= U'A' && c <= U'Z');
    }

    void Lexer::trimLeft()
    {
        while(!m_curr.empty())
        {
            auto it = m_curr.begin();
            char32_t c = utf8::next(it, m_curr.end());

            if(!isSpace(c))
                break;

            m_curr.remove_prefix(it - m_curr.begin());
        }
    }

    Token Lexer::chop(size_t n)
    {
        auto token = m_curr.substr(0, n);
        m_curr.remove_prefix(n);
        return token;
    }

    template <typename Predicate>
    Token Lexer::chopWhile(Predicate &&predicate)
    {
        size_t n = 0;
        
        while(n < m_curr.size())
        {
            size_t bytes;
            char32_t c = peek(m_curr.substr(n), bytes);

            if(!predicate(c))
                break;

            n += bytes;
        }

        return chop(n);
    }

    Lexer::Lexer(const Token &content)
        : m_curr(content) {}

    std::optional<Token> Lexer::nextToken()
    {
        chopWhile(isSpace);

        if(m_curr.empty())
            return std::nullopt;

        size_t bytes;
        char32_t c = peek(m_curr, bytes);

        if(isDigit(c))
            return chopWhile(isDigit);

        if(isAlpha(c))
            return chopWhile(isAlpha);

        return chop(bytes);
    }
}