#pragma once

#include <string>
#include <unordered_map>

#include <filesystem>

namespace seepp
{
    using TermFreq = std::unordered_map<std::string, size_t>;
    using TermFreqIndex = std::unordered_map<std::filesystem::path, TermFreq>;

    class TfIdf
    {
    public:
        double termFreq(const std::string &term, const TermFreq &tf) const;
        double inverseDocumentFrequency(const std::string &term, const TermFreqIndex &tfIndex) const;
    };
}