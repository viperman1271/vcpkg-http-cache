#pragma once

#include <accesspermission.hpp>

#include <chrono>
#include <string>

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