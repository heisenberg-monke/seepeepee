#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include <filesystem>

namespace seepp
{
    using DocFreq = std::unordered_map<std::string, size_t>;
    using TermFreq = std::unordered_map<std::string, size_t>;
    using TermFreqPerDoc = std::unordered_map<std::filesystem::path, std::pair<size_t, TermFreq>>;

    class Model
    {
        DocFreq m_df;
        TermFreqPerDoc m_tfpd;

        friend class App;
        friend class FileSystem;

    public:
        double termFreq(const std::string &term, size_t n, const TermFreq &tf) const;
        double invDocFreq(const std::string &term) const;
        std::vector<std::pair<const std::filesystem::path*, double>> search(const std::string &query) const;
    };
}