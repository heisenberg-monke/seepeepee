#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include <filesystem>
#include <mutex>

#include <sqlite3.h>

#include "Logger.hpp"

namespace seepp
{
    using DocFreq = std::unordered_map<std::string, size_t>;
    using TermFreq = std::unordered_map<std::string, size_t>;

    struct Doc
    {
        TermFreq tf;
        size_t count;
        std::filesystem::file_time_type lastModified;
    };
    
    using Docs = std::unordered_map<std::filesystem::path, Doc>;

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
        mutable std::mutex m_mutex;

    public:
        Model();
        virtual ~Model() = default;

        virtual bool needsIndexing(const std::filesystem::path &filePath, std::filesystem::file_time_type lastModified) const = 0;

        virtual void addDocument(const std::filesystem::path &filePath, const std::string &content, std::filesystem::file_time_type lastModified) = 0;
        virtual void removeDocument(const std::filesystem::path &filePath) = 0;

        virtual std::vector<std::pair<std::filesystem::path, double>> search(const std::string &query) const = 0;
        virtual std::vector<std::filesystem::path> documents() const = 0;
    };

    class InMemoryModel : public Model
    {
        DocFreq m_df;
        Docs m_docs;

        friend class App;
        friend class FileSystem;

    public:
        double termFreq(const std::string &term, size_t n, const TermFreq &tf) const;
        double invDocFreq(const std::string &term) const;

        bool needsIndexing(const std::filesystem::path &filePath, std::filesystem::file_time_type lastModified) const override;

        void addDocument(const std::filesystem::path &filePath, const std::string &content, std::filesystem::file_time_type lastModified) override;
        void removeDocument(const std::filesystem::path &filePath) override;

        std::vector<std::pair<std::filesystem::path, double>> search(const std::string &query) const override;
        std::vector<std::filesystem::path> documents() const override;
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

        void check(int rc, const std::string &operation) const;

        bool needsIndexing(const std::filesystem::path &filePath, std::filesystem::file_time_type lastModified) const override;

        void addDocument(const std::filesystem::path &filePath, const std::string &content, std::filesystem::file_time_type lastModified) override;
        void removeDocument(const std::filesystem::path &filePath) override;
        
        std::vector<std::pair<std::filesystem::path, double>> search(const std::string &query) const override;
        std::vector<std::filesystem::path> documents() const override;
    };
}