#include <apikey.hpp>

ApiKey::ApiKey(const std::string& m_Key, const std::string& description, AccessPermission permission, std::optional<std::chrono::system_clock::time_point> expiry)
    : m_Key(m_Key)
    , m_Description(description)
    , m_Permission(permission)
    , m_CreatedAt(std::chrono::system_clock::now())
    , m_ExpiresAt(expiry)
    , m_Revoked(false)
{
}