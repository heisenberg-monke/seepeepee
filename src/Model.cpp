#include "Model.hpp"
#include "Lexer.hpp"

#include <cmath>

#include <algorithm>

namespace seepp
{
    double Model::termFreq(const std::string &term, size_t n, const TermFreq &tf) const
    {
        auto it = tf.find(term);
        double freq = it == tf.end() ? 0.0 : static_cast<double>(it->second);

        return n == 0 ? 0.0 : freq / static_cast<double>(n);
    }

    double Model::invDocFreq(const std::string &term) const
    {
        auto it = m_df.find(term);
        return std::log10(static_cast<double>(m_tfpd.size()) / static_cast<double>(it != m_df.end() ? it->second : 1));
    }

    std::vector<std::pair<const std::filesystem::path*, double>> Model::search(const std::string &query) const
    {
        std::vector<std::pair<const std::filesystem::path*, double>> result;
        
        result.reserve(m_tfpd.size());

        Lexer lexer(query);
        std::vector<std::string> tokens;

        while(auto token = lexer.nextToken())
            tokens.emplace_back(token.value());

        for(const auto &[path, tf] : m_tfpd)
        {
            double rank = 0.0;

            for(const auto &token : tokens)
                rank += termFreq(token, tf.first, tf.second) * invDocFreq(token);

            result.emplace_back(&path, rank);
        }

        std::sort(result.begin(), result.end(), [](const auto &a, const auto &b) {
            return a.second > b.second;
        });

        return result;
    }
}