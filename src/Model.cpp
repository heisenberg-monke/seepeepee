#include "Model.hpp"
#include "Lexer.hpp"
#include "sqlite3.h"

#include <cmath>

#include <algorithm>
#include <cstddef>
#include <stdexcept>

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
        return std::log10(static_cast<double>(m_docs.size()) / static_cast<double>(it != m_df.end() ? it->second : 1));
    }

    std::vector<std::pair<std::filesystem::path, double>> InMemoryModel::search(const std::string &query) const
    {
        std::vector<std::pair<std::filesystem::path, double>> result;
        
        result.reserve(m_docs.size());

        Lexer lexer(query);
        std::vector<std::string> tokens;

        while(auto token = lexer.nextToken())
            tokens.emplace_back(token.value());

        for(const auto &[path, doc] : m_docs)
        {
            double rank = 0.0;

            for(const auto &token : tokens)
                rank += termFreq(token, doc.count, doc.tf) * invDocFreq(token);

            result.emplace_back(path, rank);
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

        m_docs.try_emplace(filePath, tf, count);
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
        Lexer lexer(content);

        TermFreq tf;
        size_t count = 0;

        while(auto token = lexer.nextToken())
        {
            ++tf[*token];
            ++count;
        }

        auto checkStmt = [&](int rc, const std::string &operation)
        {
            if(rc != SQLITE_OK && rc != SQLITE_DONE && rc != SQLITE_ROW)
                throw std::runtime_error(operation + ": " + sqlite3_errmsg(m_connection.get()));
        };

        sqlite3_stmt *rawStatement = nullptr;
        const std::string path = filePath.string();

        Statement statement(rawStatement);
        sqlite3_int64 docID;

        {
            const char *query =  "INSERT INTO Documents(path, term_count) VALUES(?, ?);";

            checkStmt(sqlite3_prepare_v2( m_connection.get(), query, -1, &rawStatement, nullptr), "Failed to prepare query");

            statement.reset(rawStatement);

            checkStmt(sqlite3_bind_text(statement.get(), 1, path.c_str(), static_cast<int>(path.size()), SQLITE_TRANSIENT), "Failed to bind path");
            checkStmt(sqlite3_bind_int64(statement.get(), 2, static_cast<sqlite3_int64>(count)), "Failed to bind term count");

            checkStmt(sqlite3_step(statement.get()), "Failed to execute query");

            docID = sqlite3_last_insert_rowid(m_connection.get());
        }

        for(const auto &[term, freq] : tf)
        {
            {
                const char *query = "INSERT INTO TermFreq(doc_id, term, freq) VALUES (?, ?, ?);";

                rawStatement = nullptr;

                checkStmt(sqlite3_prepare_v2(m_connection.get(), query, -1, &rawStatement, nullptr), "Failed to prepare query");
                statement.reset(rawStatement);

                checkStmt(sqlite3_bind_int64(statement.get(), 1, docID), "Failed to bind docID");
                checkStmt(sqlite3_bind_text(statement.get(), 2, term.c_str(), static_cast<int>(term.size()), SQLITE_TRANSIENT), "Failed to bind term");
                checkStmt(sqlite3_bind_int64(statement.get(), 3, freq), "Failed to bind freq");

                checkStmt(sqlite3_step(statement.get()), "Failed to execute query");
            }

            {
                sqlite3_int64 docFreq;

                {
                    const char *query = "SELECT freq FROM DocFreq WHERE term = ?;";

                    rawStatement = nullptr;

                    checkStmt(sqlite3_prepare_v2(m_connection.get(), query, -1, &rawStatement, nullptr), "Failed to prepare query");
                    statement.reset(rawStatement);

                    checkStmt(sqlite3_bind_text(statement.get(), 1, term.c_str(), static_cast<int>(term.size()), SQLITE_TRANSIENT), "Failed to bind term");

                    checkStmt(sqlite3_step(statement.get()), "Failed to execute query");

                    docFreq = sqlite3_column_int64(statement.get(), 0);
                }

                {
                    const char *query = "INSERT OR REPLACE INTO DocFreq(term, freq) VALUES (?, ?);";

                    rawStatement = nullptr;

                    checkStmt(sqlite3_prepare_v2(m_connection.get(), query, -1, &rawStatement, nullptr), "Failed to prepare query");
                    statement.reset(rawStatement);

                    checkStmt(sqlite3_bind_text(statement.get(), 1, term.c_str(), static_cast<int>(term.size()), SQLITE_TRANSIENT), "Failed to bind term");
                    checkStmt(sqlite3_bind_int64(statement.get(), 2, docFreq), "Failed to find docFreq");

                    checkStmt(sqlite3_step(statement.get()), "Failed to execute query");
                }
            }
        }
    }

    std::vector<std::pair<std::filesystem::path, double>> SQLiteModel::search(const std::string &query) const
    {
        m_logger.log() << "This isn't implemented yet, but I'm glad we've reached this far.\n";
        
        return {};
    }
}