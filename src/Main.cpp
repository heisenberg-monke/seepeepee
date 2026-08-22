#include <fstream>
#include <iostream>

#include <algorithm>

#include <unordered_map>

#include "seepeepee.hpp"

#include <nlohmann/json.hpp>

using TermFreq = std::unordered_map<std::string, size_t>;
using TermFreqIndex = std::unordered_map<std::filesystem::path, TermFreq>;

int main(int argc, char **argv)
{
    try
    {
        auto upper = [](const seepp::Token &token)
        {
            std::string result(token);

            std::transform(result.begin(), result.end(), result.begin(), [](uint8_t c) {
                return std::toupper(c);
            });

            return result;
        };

        seepp::FileSystem fs;
        size_t n = 20;
        TermFreqIndex tfIndex;
        nlohmann::json j;

        for(const auto &[path, content] : fs.loadXMLDir("docs.gl/gl4"))
        {
            seepp::Lexer lexer(content);
            TermFreq tf;

            while(auto token = lexer.nextToken())
                tf[upper(token.value())]++;
            
            std::vector<std::pair<std::string, size_t>> stats(tf.begin(), tf.end());

            std::sort(stats.begin(), stats.end(), [](const auto &a, const auto &b) {
                return a.second > b.second;
            });

            tfIndex.try_emplace(path, tf);

            std::cout << path << " has " << tf.size() << " unique tokens.\n";
        }

        for(const auto &[path, tf] : tfIndex)
            j[path.string()] = tf;

        std::ofstream out(std::filesystem::path(PROJECT_ROOT) / "index.json");

        out << j.dump(2);
    }

    catch(const std::exception &e)
    {
        std::cerr << "[ERROR] " << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return 0;
}