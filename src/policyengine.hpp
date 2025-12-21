#pragma once

#include <persistence.hpp>

#include <chrono>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <string>

/**
 * @brief Enum representing access permissions for API keys
 */
enum class AccessPermission 
{
    READ,      // Read-only access (GET, HEAD)
    WRITE,     // Write-only access (PUT)
    READWRITE  // Full access (GET, HEAD, PUT)
};

/**
 * @brief Structure representing an API key with metadata
 */
struct ApiKey 
{
public:
    ApiKey(const std::string& key, const std::string& desc, AccessPermission perm, std::optional<std::chrono::system_clock::time_point> expiry = std::nullopt);

    bool GetIsRevoked() const { return m_Revoked; }
    void Revoke() { m_Revoked = true; }

    AccessPermission GetPermission() const { return m_Permission; }
    const std::string& GetKey() const { return m_Key; }
    const std::string& GetDescription() const { return m_Description; }

    const std::optional<std::chrono::system_clock::time_point>& GetExpiry() const { return m_ExpiresAt; }

private:
    bool m_Revoked;
    AccessPermission m_Permission;
    std::string m_Key;
    std::string m_Description;
    std::chrono::system_clock::time_point m_CreatedAt;

    std::optional<std::chrono::system_clock::time_point> m_ExpiresAt;
};

class PolicyEngine final
{
public:
    PolicyEngine(PersistenceInfo& persistenceInfo);

    /**
     * @brief Create a new API key with specified permissions
     *
     * @param description Human-readable description of the key
     * @param permission Access permission level
     * @param expires_in_days Optional expiration period in days (0 = no expiration)
     * @return std::string The generated API key
     */
    std::string CreateApiKey(const std::string& description, AccessPermission permission, std::optional<std::chrono::days> expiry = std::nullopt);

    /**
     * @brief Revoke an API key
     *
     * @param api_key The API key to revoke
     * @return true if the key was found and revoked
     * @return false if the key was not found
     */
    bool RevokeApiKey(const std::string& apiKey);

    /**
     * @brief Validate an API key from HTTP header has appropriate permissions
     *
     * @param apiKey The API key from the request header
     * @param requestedPermission The permission requested with the API key
     * @return bool indicating if the key is valid and has appropriate permissions
     */
    bool ValidateApiKey(const std::string& apiKey, AccessPermission requestedPermission) const;

    /**
     * @brief Validate an API key from HTTP header
     *
     * @param apiKey The API key from the request header
     * @return bool Indicating whether the key is valid
     */
    bool ValidateApiKey(const std::string& apiKey) const;

    /**
     * @brief Validate if an API key is expired
     *
     * @param apiKey The API key from the request header
     * @return bool Indicating whether an API key is expired
     */
    bool IsExpired(const std::string& apiKey) const;

    /**
     * @brief Clean up expired API keys
     *
     * @return size_t Number of keys removed
     */
    size_t CleanupExpiredKeys();

private:
    /**
     * @brief Generate a cryptographically secure random API key
     *
     * @return std::string The generated key
     */
    static std::string GenerateKey();

    /**
     * @brief Check if a key has expired
     *
     * @param key The API key to check
     * @return true if expired
     */
    bool IsExpired(const ApiKey& key) const;

    /**
     * @brief Check if HTTP method is allowed for given permission
     *
     * @param permission The access permission level
     * @param http_method The HTTP method
     * @return true if allowed
     */
    bool IsMethodAllowed(AccessPermission permission, const std::string& httpMethod);

private:
    std::unordered_map<std::string, ApiKey> m_ApiKeys;
    PersistenceInfo& m_PersistenceInfo;
    mutable std::recursive_mutex m_Mutex;
};

/**
 * @brief Convert AccessPermission to string
 */
std::string ToString(AccessPermission perm);

/**
 * @brief Convert string to AccessPermission
 */
std::optional<AccessPermission> FromString(const std::string& str);