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

ApiKey::ApiKey(const nlohmann::json& json)
{
    Load(json);
}

void ApiKey::Save(nlohmann::json& json) const
{
    json = nlohmann::json
    {
        { "revoked", m_Revoked },
        { "permission", static_cast<uint32_t>(m_Permission) },
        { "key", m_Key },
        { "description", m_Description },
        { "created", std::chrono::duration_cast<std::chrono::seconds>(m_CreatedAt.time_since_epoch()).count() }
    };

    if (m_ExpiresAt.has_value())
    {
        json["expires"] = std::chrono::duration_cast<std::chrono::seconds>(m_ExpiresAt.value().time_since_epoch()).count();
    }
}

void ApiKey::Load(const nlohmann::json& json)
{
    m_Revoked = json.at("revoked").get<bool>();
    m_Permission = static_cast<AccessPermission>(json.at("permission").get<uint32_t>());
    m_Key = json.at("key").get<std::string>();
    m_Description = json.at("description").get<std::string>();
    m_CreatedAt = std::chrono::system_clock::time_point{std::chrono::seconds{ json.at("created").get<uint64_t>() }};

    if (json.contains("expires"))
    {
        m_ExpiresAt = std::chrono::system_clock::time_point{ std::chrono::seconds{ json.at("expires").get<uint64_t>() } };
    }
}
