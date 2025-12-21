#pragma once

#include <persistence.hpp>

#include <drogon/HttpController.h>
#include <drogon/HttpTypes.h>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <memory>
#include <string>

class ApiKeyFilter;
class PolicyEngine;

class BinaryCacheServer : public drogon::HttpController<BinaryCacheServer, false> 
{
public:
    METHOD_LIST_BEGIN
    // HEAD request to check if a binary package exists
    ADD_METHOD_TO(BinaryCacheServer::CheckPackage, "/{triplet}/{name}/{version}/{sha}", drogon::Head, "ApiKeyFilter");
    
    // GET request to download a binary package
    ADD_METHOD_TO(BinaryCacheServer::GetPackage, "/{triplet}/{name}/{version}/{sha}", drogon::Get, "ApiKeyFilter");
    
    // PUT request to upload a binary package
    ADD_METHOD_TO(BinaryCacheServer::PutPackage, "/{triplet}/{name}/{version}/{sha}", drogon::Put, "ApiKeyFilter");
    
    // GET server status
    ADD_METHOD_TO(BinaryCacheServer::GetStatus, "/status", drogon::Get, "ApiKeyFilter");

    // GET method to terminate server via IPC
    ADD_METHOD_TO(BinaryCacheServer::Kill, "/internal/kill", drogon::Get, "drogon::LocalHostFilter");

    // Create new API key
    ADD_METHOD_TO(BinaryCacheServer::CreateKey, "/api/keys", drogon::Post, "drogon::LocalHostFilter");

    // Get specific API key info
    ADD_METHOD_TO(BinaryCacheServer::GetKeyInfo, "/api/keys/{key}", drogon::Get, "drogon::LocalHostFilter");

    // Revoke API key
    ADD_METHOD_TO(BinaryCacheServer::RevokeKey, "/api/keys/{key}", drogon::Delete, "drogon::LocalHostFilter");

    // Cleanup expired keys
    ADD_METHOD_TO(BinaryCacheServer::CleanupExpired, "/api/keys/cleanup", drogon::Post, "drogon::LocalHostFilter");
    
    METHOD_LIST_END

    /**
     * @brief Constructor
     * @param cacheDir Directory to store cached binary packages
     */
    explicit BinaryCacheServer(const std::string& cacheDir, const std::string& persistenceFile);

    /**
     * @brief Check if a package exists (HEAD request)
     * @param req HTTP request
     * @param callback Callback function
     * @param triplet Target triplet (e.g., x64-linux, x64-windows)
     * @param name Package name
     * @param version Package version
     * @param sha Package SHA hash
     */
    void CheckPackage(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, const std::string& triplet, const std::string& name, const std::string& version, const std::string& sha) const;

    /**
     * @brief Download a package (GET request)
     * @param req HTTP request
     * @param callback Callback function
     * @param triplet Target triplet (e.g., x64-linux, x64-windows)
     * @param name Package name
     * @param version Package version
     * @param sha Package SHA hash
     */
    void GetPackage(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, const std::string& triplet, const std::string& name, const std::string& version, const std::string& sha) const;

    /**
     * @brief Upload a package (PUT request)
     * @param req HTTP request
     * @param callback Callback function
     * @param triplet Target triplet (e.g., x64-linux, x64-windows)
     * @param name Package name
     * @param version Package version
     * @param sha Package SHA hash
     */
    void PutPackage(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, const std::string& triplet, const std::string& name, const std::string& version, const std::string& sha);

    /**
     * @brief Get server status
     * @param req HTTP request
     * @param callback Callback function
     */
    void GetStatus(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) const;

    /**
     * @brief Set the cache directory
     * @param dir Directory path
     */
    void SetCacheDirectory(const std::string& dir);

    /**
     * @brief Get the cache directory
     * @return Cache directory path
     */
    std::string GetCacheDirectory() const { return m_CacheDir.string(); }

    /**
     * @brief Creates an instance of the ApiKeyFilter
     * @return std::shared_ptr instance of ApiKeyFilter
     */
    std::shared_ptr<ApiKeyFilter> CreateApiKeyFilter(bool requireAuthForRead = false, bool requireAuthForWrite = false, bool requireAuthForStatus = false) const;

    /**
     * @brief Create a new API key
     *
     * Request body (JSON):
     * {
     *   "description": "Key for CI/CD pipeline",
     *   "permission": "readwrite",  // "read", "write", or "readwrite"
     *   "expires_in_days": 365       // Optional, 0 = no expiration
     * }
     */
    void CreateKey(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    /**
     * @brief Get information about a specific API key
     */
    void GetKeyInfo(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, const std::string& key) const;

    /**
     * @brief Revoke an API key
     */
    void RevokeKey(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, const std::string& key);

    /**
     * @brief Clean up expired API keys
     */
    void CleanupExpired(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);

private:
    /**
     * @brief Get server status
     * @param req HTTP request
     * @param callback Callback function
     */
    void Kill(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) const;

    /**
     * @brief Get the file path for a given package
     * @param triplet Target triplet
     * @param name Package name
     * @param version Package version
     * @param sha Package SHA hash
     * @return Full file path
     */
    std::filesystem::path GetPackagePath(const std::string& triplet, const std::string& name, const std::string& version, const std::string& sha) const;

    /**
     * @brief Validate hash format
     * @param hash Hash string to validate
     * @return true if valid, false otherwise
     */
    bool IsValidHash(const std::string& hash) const;

    /**
     * @brief Get cache statistics
     * @return JSON object with cache stats
     */
    nlohmann::json GetCacheStats() const;

    /**
     * @brief Convert ApiKey to JSON object
     */
    nlohmann::json ApiKeyToJson(const ApiKey& key) const;

    /**
     * @brief Send exception information as a JSON response
     */
    void SendExceptionAsJson(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, const std::exception& e) const;

private:
    std::filesystem::path m_CacheDir;

    mutable PersistenceInfo m_PersistenceInfo;
    std::shared_ptr<PolicyEngine> m_PolicyEngine;
};