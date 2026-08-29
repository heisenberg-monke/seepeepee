#include "Lexer.hpp"

#include <vector>

#include <unicode/utf8.h>
#include <unicode/unistr.h>
#include <unicode/ustring.h>
#include <unicode/uchar.h>

#include <stdexcept>

namespace seepp
{
    static inline std::pair<UChar32, size_t> decodeUTF8(const Token &token)
    {
        UChar32 c;
        UChar32 i = 0;

        U8_NEXT(reinterpret_cast<const uint8_t *>(token.data()), i, static_cast<UChar32>(token.size()), c);

        return {c, static_cast<size_t>(i)};
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
            auto [c, width] = decodeUTF8(m_curr.substr(n));

            if(!predicate(c))
                break;

            n += width;
        }

        return chop(n);
    }

    bool isValid(const Token &content)
    {
        UErrorCode status = U_ZERO_ERROR;
        int32_t length = 0;
        
        u_strFromUTF8(nullptr, 0, &length, content.data(), static_cast<int32_t>(content.size()), &status);

        if(status != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(status))
            return false;

        status = U_ZERO_ERROR;

        std::vector<UChar> buffer(length);

        u_strFromUTF8(buffer.data(), length, nullptr, content.data(), static_cast<int32_t>(content.size()), &status);

        return U_SUCCESS(status);
    }

    Lexer::Lexer(const Token &content)
        : m_curr(content)
    {
        if(!isValid(content))
            throw std::runtime_error("Invalid UTF8");
    }

    std::optional<std::string> Lexer::nextToken()
    {
        chopWhile(u_isUWhiteSpace);

        if(m_curr.empty())
            return std::nullopt;

        auto [c, width] = decodeUTF8(m_curr);

        if(u_getIntPropertyValue(c, UCHAR_NUMERIC_TYPE) != U_NT_NONE)
            return std::string(chopWhile([](UChar32 c)
            {
                return u_getIntPropertyValue(c, UCHAR_NUMERIC_TYPE) != U_NT_NONE;
            }));

        if(u_isalpha(c))
        {
            auto token = chopWhile(u_isalnum);
            auto unicode = icu::UnicodeString::fromUTF8(token);

            std::string result;

            unicode.toUpper();
            unicode.toUTF8String(result);

            return result;
        }

        return std::string(chop(width));
    }
}