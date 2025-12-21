#include <server.hpp>

#include <filters/authfilter.hpp>
#include <policyengine.hpp>

#include <drogon/HttpResponse.h>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>

BinaryCacheServer::BinaryCacheServer(const std::string& cacheDir, const std::string& persistenceFile)
    : m_CacheDir(cacheDir) 
{
    // Create cache directory if it doesn't exist
    if (!std::filesystem::exists(m_CacheDir)) 
    {
        std::filesystem::create_directories(m_CacheDir);
    }

    m_PolicyEngine = std::make_shared<PolicyEngine>(m_PersistenceInfo);

    m_PersistenceInfo.SetPersistencePath(persistenceFile);
    m_PersistenceInfo.Load();
}

void BinaryCacheServer::CheckPackage(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, const std::string& triplet, const std::string& name, const std::string& version, const std::string& sha) const 
{
    m_PersistenceInfo.IncreaseTotalRequests();

    // Validate SHA
    if (!IsValidHash(sha)) 
    {
        drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        resp->setBody("Invalid SHA format");
        callback(resp);
        return;
    }

    // Check if package exists
    const std::filesystem::path packagePath = GetPackagePath(triplet, name, version, sha);
    if (std::filesystem::exists(packagePath)) 
    {
        drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k200OK);
        
        // Add content length header
        const uint64_t fileSize = std::filesystem::file_size(packagePath);
        resp->addHeader("Content-Length", std::to_string(fileSize));
        resp->addHeader("Content-Type", "application/zip");
        
        callback(resp);
    } 
    else 
    {
        drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k404NotFound);
        callback(resp);
    }
}

void BinaryCacheServer::GetPackage(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, const std::string& triplet, const std::string& name, const std::string& version, const std::string& sha) const 
{
    m_PersistenceInfo.IncreaseTotalRequests();
    m_PersistenceInfo.GetDownloads();

    // Validate SHA
    if (!IsValidHash(sha)) 
    {
        drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        resp->setBody("Invalid SHA format");
        callback(resp);
        return;
    }

    // Get package path
    const std::filesystem::path packagePath = GetPackagePath(triplet, name, version, sha);
    
    if (!std::filesystem::exists(packagePath)) 
    {
        drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k404NotFound);
        resp->setBody("Package not found");
        callback(resp);
        return;
    }

    // Read file
    try 
    {
        std::ifstream file(packagePath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) 
        {
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k500InternalServerError);
            resp->setBody("Failed to open package file");
            callback(resp);
            return;
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> buffer(size);
        if (!file.read(buffer.data(), size)) 
        {
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k500InternalServerError);
            resp->setBody("Failed to read package file");
            callback(resp);
            return;
        }

        // Create response with file content
        drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k200OK);
        resp->setBody(std::string(buffer.begin(), buffer.end()));
        resp->addHeader("Content-Type", "application/zip");
        resp->addHeader("Content-Disposition", "attachment; filename=\"" + name + "-" + version + "-" + triplet + ".zip\"");
        
        callback(resp);
    } 
    catch (const std::exception& e) 
    {
        drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->setBody(std::string("Error reading package: ") + e.what());
        callback(resp);
    }
}

void BinaryCacheServer::PutPackage(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, const std::string& triplet, const std::string& name, const std::string& version, const std::string& sha) 
{
    m_PersistenceInfo.IncreaseTotalRequests();
    m_PersistenceInfo.GetUploads();

    if (!IsValidHash(sha)) 
    {
        drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        resp->setBody("Invalid SHA format");
        callback(resp);
        return;
    }

    const std::string_view body = req->getBody();
    if (body.empty()) 
    {
        drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        resp->setBody("Empty request body");
        callback(resp);
        return;
    }

    const std::filesystem::path packagePath = GetPackagePath(triplet, name, version, sha);
    
    try 
    {
        // Create parent directories if needed
        const std::filesystem::path parentPath = packagePath.parent_path();
        if (!std::filesystem::exists(parentPath)) 
        {
            std::filesystem::create_directories(parentPath);
        }

        std::ofstream file(packagePath, std::ios::binary);
        if (!file.is_open()) 
        {
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k500InternalServerError);
            resp->setBody("Failed to create package file");
            callback(resp);
            return;
        }

        file.write(body.data(), body.size());
        file.close();

        // Success response
        drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k201Created);
        
        nlohmann::json json;
        json["status"] = "success";
        json["triplet"] = triplet;
        json["name"] = name;
        json["version"] = version;
        json["sha"] = sha;
        json["size"] = body.size();
        json["message"] = "Package uploaded successfully";
        
        resp->setBody(nlohmann::to_string(json));
        resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        
        callback(resp);
    } 
    catch (const std::exception& e) 
    {
        drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->setBody(std::string("Error writing package: ") + e.what());
        callback(resp);
    }
}

void BinaryCacheServer::GetStatus(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) const 
{
    m_PersistenceInfo.IncreaseTotalRequests();

    try 
    {
        const nlohmann::json stats = GetCacheStats();
        
        drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k200OK);
        resp->setBody(nlohmann::to_string(stats));
        resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        
        callback(resp);
    } 
    catch (const std::exception& e) 
    {
        drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->setBody(std::string("Error getting status: ") + e.what());
        callback(resp);
    }
}

void BinaryCacheServer::SetCacheDirectory(const std::string& dir) 
{
    m_CacheDir = dir;
    if (!std::filesystem::exists(m_CacheDir)) 
    {
        std::filesystem::create_directories(m_CacheDir);
    }
}

std::shared_ptr<ApiKeyFilter> BinaryCacheServer::CreateApiKeyFilter(bool requireAuthForRead, bool requireAuthForWrite, bool requireAuthForStatus) const
{
    return std::make_shared<ApiKeyFilter>(m_PolicyEngine, requireAuthForRead, requireAuthForWrite, requireAuthForStatus);
}

void BinaryCacheServer::CreateKey(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    try 
    {
        // Parse request body
        const nlohmann::json jsonBody = nlohmann::json::parse(req->body(), nullptr, false);
        if (jsonBody.is_discarded())
        {
            const nlohmann::json error
            {
                { "error", "Invalid request" },
                { "message", "Request body must be valid JSON" }
            };

            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k400BadRequest);
            resp->setBody(nlohmann::to_string(error));
            resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);

            callback(resp);
            return;
        }

        // Extract parameters
        const std::string description = jsonBody.contains("description") ? jsonBody.at("description").get<std::string>() : "";
        const std::string permissionString = jsonBody.contains("permission") ? jsonBody.at("permission").get<std::string>() : "read";
        std::optional<std::chrono::days> expiresInDays;
        if (jsonBody.contains("expiresInDays"))
        {
            expiresInDays = std::chrono::days(jsonBody.at("expiresInDays").get<uint32_t>());
        }

        // Validate permission
        const std::optional<AccessPermission> permission = FromString(permissionString);
        if (!permission.has_value()) 
        {
            const nlohmann::json error
            {
                { "error", "Invalid permission" },
                { "message", "Permission must be 'read', 'write', or 'readwrite'" }
            };

            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k400BadRequest);
            resp->setBody(nlohmann::to_string(error));
            resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);

            callback(resp);
        }
        else
        {
            // Create the API key
            const std::string newKey = m_PolicyEngine->CreateApiKey(description, permission.value(), expiresInDays);

            const nlohmann::json response
            {
                { "success", true },
                { "message", "API key created successfully" },
                { "apiKey", newKey }
            };

            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k201Created);
            resp->setBody(nlohmann::to_string(response));
            resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);

            callback(resp);
        }
    }
    catch (const std::exception& e) 
    {
        SendExceptionAsJson(req, std::move(callback), e);
    }
}

void BinaryCacheServer::GetKeyInfo(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, const std::string& key) const
{
    try 
    {
        const std::optional<ApiKey> keyInfo = m_PolicyEngine->GetApiKey(key);

        if (!keyInfo.has_value())
        {
            const nlohmann::json error
            {
                { "error", "Not found" },
                { "message", "API key not found" }
            };

            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k500InternalServerError);
            resp->setBody(nlohmann::to_string(error));
            resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);

            callback(resp);
        }
        else
        {

            const nlohmann::json response
            {
                { "success", true },
                { "apiKey", ApiKeyToJson(keyInfo.value()) }
            };

            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k500InternalServerError);
            resp->setBody(nlohmann::to_string(response));
            resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);

            callback(resp);
        }
    }
    catch (const std::exception& e) 
    {
        SendExceptionAsJson(req, std::move(callback), e);
    }
}

void BinaryCacheServer::RevokeKey(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, const std::string& key)
{
    try 
    {
        const bool revoked = m_PolicyEngine->RevokeApiKey(key);

        if (!revoked) 
        {
            const nlohmann::json error
            {
                { "error", "Not found" },
                { "message", "API key not found" }
            };

            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k404NotFound);
            resp->setBody(nlohmann::to_string(error));
            resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);

            callback(resp);
        }
        else
        {
            const nlohmann::json response
            {
                { "success", true },
                { "message", "API key revoked successfully" }
            };

            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k200OK);
            resp->setBody(nlohmann::to_string(response));
            resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);

            callback(resp);
        }
    }
    catch (const std::exception& e) 
    {
        SendExceptionAsJson(req, std::move(callback), e);
    }
}

void BinaryCacheServer::CleanupExpired(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    try 
    {
        const size_t count = m_PolicyEngine->CleanupExpiredKeys();

        const nlohmann::json response
        {
            { "success", true },
            { "message", "Expired keys cleaned up" },
            { "removedCount", count },
        };

        drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k200OK);
        resp->setBody(nlohmann::to_string(response));
        resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);

        callback(resp);
    }
    catch (const std::exception& e) 
    {
        SendExceptionAsJson(req, std::move(callback), e);
    }
}

void BinaryCacheServer::Kill(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) const
{
    drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k200OK);

    drogon::app().quit();

    callback(resp);
}

std::filesystem::path BinaryCacheServer::GetPackagePath(const std::string& triplet, const std::string& name, const std::string& version, const std::string& sha) const 
{
    // Store packages in the vcpkg structure: triplet/name/version/sha.zip
    return m_CacheDir / triplet / name / version / (sha + ".zip");
}

bool BinaryCacheServer::IsValidHash(const std::string& hash) const 
{
    // Hash should be alphanumeric and reasonable length (e.g., SHA256 = 64 chars)
    if (hash.empty() || hash.length() > 128) 
    {
        return false;
    }

    return std::all_of(hash.begin(), hash.end(), [](char c) 
    {
        return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-';
    });
}

nlohmann::json BinaryCacheServer::GetCacheStats() const
{
    nlohmann::json stats;
    
    stats["service"] = "vcpkg-binary-cache-server";
    stats["version"] = "1.0.0";
    stats["cache_directory"] = m_CacheDir.string();
    
    // Count packages
    uint64_t packageCount = 0;
    uint64_t totalSize = 0;
    
    if (std::filesystem::exists(m_CacheDir)) 
    {
        for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(m_CacheDir))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".zip") 
            {
                ++packageCount;
                totalSize += std::filesystem::file_size(entry.path());
            }
        }
    }
    
    stats["package_count"] = packageCount;
    stats["total_size_bytes"] = totalSize;
    stats["total_size_mb"] = std::round(static_cast<double>((totalSize) / (1024.0 * 1024.0)) * 100.0) / 100.0;
    
    stats["statistics"]["total_requests"] = m_PersistenceInfo.GetTotalRequests();
    stats["statistics"]["uploads"] = m_PersistenceInfo.GetUploads();
    stats["statistics"]["downloads"] = m_PersistenceInfo.GetDownloads();
    
    return stats;
}

nlohmann::json BinaryCacheServer::ApiKeyToJson(const ApiKey& key) const
{
    nlohmann::json json
    {
        { "description", key.GetDescription() },
        { "permission", ToString(key.GetPermission()) },
        { "createdAt", std::chrono::system_clock::to_time_t(key.GetCreatedAt()) },
        { "revoked", key.GetIsRevoked() }
    };

    if (key.GetExpiry().has_value())
    {
        json["expiresAt"] = std::chrono::system_clock::to_time_t(key.GetExpiry().value());
    }

    return json;
}

void BinaryCacheServer::SendExceptionAsJson(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, const std::exception& e) const
{
    const nlohmann::json error
    {
        { "error", "Internal server error" },
        { "message", e.what() }
    };

    drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k500InternalServerError);
    resp->setBody(nlohmann::to_string(error));
    resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);

    callback(resp);
}
