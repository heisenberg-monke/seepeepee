#include "Model.hpp"
#include "Lexer.hpp"

#include <cmath>

#include <algorithm>

namespace seepp
{
    double Model::termFreq(const std::string &term, const TermFreq &tf) const
    {
        size_t sum = 0;

        for(const auto &[t, f] : tf)
            sum += f;

        auto it = tf.find(term);
        double freq = it == tf.end() ? 0.0 : static_cast<double>(it->second);

        return sum == 0 ? 0.0 : freq / static_cast<double>(sum);
    }

    double Model::invDocFreq(const std::string &term, const TermFreqIndex &tfIndex) const
    {
        size_t count = 0;

        for(const auto &[_, tf] : tfIndex)
            count += tf.count(term);

        return std::log(static_cast<double>(tfIndex.size()) / static_cast<double>(std::max<size_t>(count, 1)));
    }

    std::vector<std::pair<const std::filesystem::path*, double>> Model::search(TermFreqIndex &tfIndex, const std::string &query) const
    {
        std::vector<std::pair<const std::filesystem::path*, double>> result;
        
        result.reserve(tfIndex.size());

        Lexer lexer(query);
        std::vector<std::string> tokens;

        while(auto token = lexer.nextToken())
            tokens.emplace_back(token.value());

        for(const auto &[path, tf] : tfIndex)
        {
            double rank = 0.0;
            Lexer lexer(query);

            for(const auto &token : tokens)
                rank += termFreq(token, tf) * invDocFreq(token, tfIndex);

            result.emplace_back(&path, rank);
        }

        std::sort(result.begin(), result.end(), [](const auto &a, const auto &b) {
            return a.second > b.second;
        });

        return result;
    }
}