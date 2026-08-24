#include "TfIdf.hpp"

#include <cmath>

namespace seepp
{
    double TfIdf::termFreq(const std::string &term, const TermFreq &tf) const
    {
        size_t sum = 0;

        for(const auto &[t, f] : tf)
            sum += f;

        auto it = tf.find(term);
        double freq = it == tf.end() ? 0.0 : static_cast<double>(it->second);

        return sum == 0 ? 0.0 : freq / static_cast<double>(sum);
    }

    double TfIdf::inverseDocumentFrequency(const std::string &term, const TermFreqIndex &tfIndex) const
    {
        size_t count = 0;

        for(const auto &[_, tf] : tfIndex)
            count += tf.count(term);

        return std::log(static_cast<double>(tfIndex.size()) / static_cast<double>(count));
    }
}