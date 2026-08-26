#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include <filesystem>

namespace seepp
{
    using TermFreq = std::unordered_map<std::string, size_t>;
    using TermFreqIndex = std::unordered_map<std::filesystem::path, TermFreq>;

    class Model
    {
    public:
        double termFreq(const std::string &term, const TermFreq &tf) const;
        double invDocFreq(const std::string &term, const TermFreqIndex &tfIndex) const;
        std::vector<std::pair<const std::filesystem::path*, double>> search(TermFreqIndex &tfIndex, const std::string &query) const;
    };
}