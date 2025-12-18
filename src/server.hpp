#pragma once

#include <drogon/HttpController.h>
#include <drogon/HttpTypes.h>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <memory>
#include <string>

class BinaryCacheServer : public drogon::HttpController<BinaryCacheServer, false> 
{
public:
    METHOD_LIST_BEGIN
    // HEAD request to check if a binary package exists
    ADD_METHOD_TO(BinaryCacheServer::CheckPackage, "/{triplet}/{name}/{version}/{sha}", drogon::Head);
    
    // GET request to download a binary package
    ADD_METHOD_TO(BinaryCacheServer::GetPackage, "/{triplet}/{name}/{version}/{sha}", drogon::Get);
    
    // PUT request to upload a binary package
    ADD_METHOD_TO(BinaryCacheServer::PutPackage, "/{triplet}/{name}/{version}/{sha}", drogon::Put);
    
    // GET server status
    ADD_METHOD_TO(BinaryCacheServer::GetStatus, "/status", drogon::Get);
    
    METHOD_LIST_END

    /**
     * @brief Constructor
     * @param cacheDir Directory to store cached binary packages
     */
    explicit BinaryCacheServer(const std::string& cacheDir = "./cache");

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
    std::string getCacheDirectory() const { return m_cacheDir.string(); }

private:
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

private:
    std::filesystem::path m_cacheDir;
    mutable std::atomic<uint64_t> m_requestCount{0};
    mutable std::atomic<uint64_t> m_uploadCount{0};
    mutable std::atomic<uint64_t> m_downloadCount{0};
};