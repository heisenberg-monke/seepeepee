#include "TfIdf.hpp"

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
}