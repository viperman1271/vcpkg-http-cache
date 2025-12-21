#pragma once

#include <accesspermission.hpp>

#include <nlohmann/json.hpp>

#include <chrono>
#include <string>

/**
 * @brief Structure representing an API key with metadata
 */
struct ApiKey
{
public:
    ApiKey(const std::string& key, const std::string& desc, AccessPermission perm, std::optional<std::chrono::system_clock::time_point> expiry = std::nullopt);
    ApiKey(const nlohmann::json& json);

    bool GetIsRevoked() const { return m_Revoked; }
    void Revoke() { m_Revoked = true; }

    AccessPermission GetPermission() const { return m_Permission; }
    const std::string& GetKey() const { return m_Key; }
    const std::string& GetDescription() const { return m_Description; }

    const std::optional<std::chrono::system_clock::time_point>& GetExpiry() const { return m_ExpiresAt; }

    void Save(nlohmann::json& json) const;
    void Load(const nlohmann::json& json);

private:
    bool m_Revoked;
    AccessPermission m_Permission;
    std::string m_Key;
    std::string m_Description;
    std::chrono::system_clock::time_point m_CreatedAt;

    std::optional<std::chrono::system_clock::time_point> m_ExpiresAt;
};