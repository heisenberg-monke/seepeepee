#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include <filesystem>

#include <sqlite3.h>

#include "Logger.hpp"

namespace seepp
{
    using DocFreq = std::unordered_map<std::string, size_t>;
    using TermFreq = std::unordered_map<std::string, size_t>;
    using TermFreqPerDoc = std::unordered_map<std::filesystem::path, std::pair<size_t, TermFreq>>;

    struct SQLiteDeleter
    {
        void operator()(sqlite3 *db) const noexcept;
        void operator()(sqlite3_stmt *stmt) const noexcept;
    };

    using Connection = std::unique_ptr<sqlite3, SQLiteDeleter>;
    using Statement = std::unique_ptr<sqlite3_stmt, SQLiteDeleter>;

    class Model
    {
    protected:
        Logger &m_logger;

    public:
        Model();
        virtual ~Model() = default;

        virtual void addDocument(const std::filesystem::path &filePath, const std::string &content) = 0;
        virtual std::vector<std::pair<const std::filesystem::path*, double>> search(const std::string &query) const = 0;
    };

    class InMemoryModel : public Model
    {
        DocFreq m_df;
        TermFreqPerDoc m_tfpd;

        friend class App;
        friend class FileSystem;

    public:
        double termFreq(const std::string &term, size_t n, const TermFreq &tf) const;
        double invDocFreq(const std::string &term) const;

        void addDocument(const std::filesystem::path &filePath, const std::string &content) override;
        std::vector<std::pair<const std::filesystem::path*, double>> search(const std::string &query) const override;
    };

    class SQLiteModel : public Model
    {
        Connection m_connection;

        friend class App;
        friend class FileSystem;

        bool execute(const std::string &statement) const;

    public:
        bool begin() const;
        bool commit() const;

        void addDocument(const std::filesystem::path &filePath, const std::string &content) override;
        std::vector<std::pair<const std::filesystem::path*, double>> search(const std::string &query) const override;
    };
}