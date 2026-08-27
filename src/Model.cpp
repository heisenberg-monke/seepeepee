#include "Model.hpp"
#include "Lexer.hpp"

#include <cmath>

#include <algorithm>

namespace seepp
{
    void SQLiteDeleter::operator()(sqlite3 *db) const noexcept
    {
        if(db)
            sqlite3_close(db);
    }

    void SQLiteDeleter::operator()(sqlite3_stmt *stmt) const noexcept
    {
        if(stmt)
            sqlite3_finalize(stmt);
    }

    Model::Model()
        : m_logger(Logger::getLogger()) {}
        
    double InMemoryModel::termFreq(const std::string &term, size_t n, const TermFreq &tf) const
    {
        auto it = tf.find(term);
        double freq = it == tf.end() ? 0.0 : static_cast<double>(it->second);

        return n == 0 ? 0.0 : freq / static_cast<double>(n);
    }

    double InMemoryModel::invDocFreq(const std::string &term) const
    {
        auto it = m_df.find(term);
        return std::log10(static_cast<double>(m_tfpd.size()) / static_cast<double>(it != m_df.end() ? it->second : 1));
    }

    std::vector<std::pair<const std::filesystem::path*, double>> InMemoryModel::search(const std::string &query) const
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

    void InMemoryModel::addDocument(const std::filesystem::path &filePath, const std::string &content)
    {
        TermFreq tf;
        Lexer lexer(content);

        size_t count = 0;

        while(auto token = lexer.nextToken())
        {
            tf[token.value()]++;
            ++count;
        }
        
        for(const auto &[term, _] : tf)
            m_df[term]++;

        m_tfpd.try_emplace(filePath, count, tf);
    }

    bool SQLiteModel::execute(const std::string &statement) const
    {
        char *error;
        const int res = sqlite3_exec(m_connection.get(), statement.c_str(), nullptr, nullptr, &error);

        if(res != SQLITE_OK)
        {
            m_logger.err() << "Could not execute query: " << statement << ": " << (error ? error : "unknown error") << '\n';
            sqlite3_free(error);

            return false;
        }

        return true;
    }

    bool SQLiteModel::begin() const {
        return execute("BEGIN;");
    }

    bool SQLiteModel::commit() const {
        return execute("COMMIT;");
    }

    void SQLiteModel::addDocument(const std::filesystem::path &filePath, const std::string &content)
    {
        std::string query = R"(
            INSERT INTO DOCUMENTS (path, term_count) 
            VALUES (:path, :count)
        )";

        sqlite3_stmt *raw = nullptr;

        if(sqlite3_prepare_v2(m_connection.get(), query.c_str(), -1, &raw, nullptr) != SQLITE_OK)
            throw std::runtime_error("Could not execute query " + query + ": " + sqlite3_errmsg(m_connection.get()) + '\n');

        Statement stmt(raw);
        const auto pathString = filePath.string();

        if(sqlite3_bind_text(stmt.get(), sqlite3_bind_parameter_index(stmt.get(), ":path"), pathString.c_str(), static_cast<int>(pathString.size()), SQLITE_TRANSIENT) != SQLITE_OK)
            throw std::runtime_error("Could not bind path: " + std::string(sqlite3_errmsg(m_connection.get())) + '\n');

        size_t termCount = 0;
        Lexer lexer(content);

        while(auto token = lexer.nextToken())
            ++termCount;

        if(sqlite3_bind_int64(stmt.get(), sqlite3_bind_parameter_index(stmt.get(), ":count"), static_cast<sqlite3_int64>(termCount)) != SQLITE_OK)
            throw std::runtime_error("Could not bind count: " + std::string(sqlite3_errmsg(m_connection.get()) + '\n'));

        if(sqlite3_step(stmt.get()) != SQLITE_DONE)
            throw std::runtime_error("Couuld not execute query: " + query + ": " + sqlite3_errmsg(m_connection.get()));
    }

    std::vector<std::pair<const std::filesystem::path*, double>> SQLiteModel::search(const std::string &query) const
    {
        return {};
    }
}