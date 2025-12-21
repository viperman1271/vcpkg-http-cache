#include <policyengine.hpp>

#include <random>

ApiKey::ApiKey(const std::string& m_Key, const std::string& description, AccessPermission permission, std::optional<std::chrono::system_clock::time_point> expiry)
    : m_Key(m_Key)
    , m_Description(description)
    , m_Permission(permission)
    , m_CreatedAt(std::chrono::system_clock::now())
    , m_ExpiresAt(expiry)
    , m_Revoked(false)
{
}

PolicyEngine::PolicyEngine(PersistenceInfo& persistenceInfo)
    : m_PersistenceInfo(persistenceInfo)
{
}

std::string PolicyEngine::CreateApiKey(const std::string& description, AccessPermission permission, std::optional<std::chrono::days> expiry /*= std::nullopt*/)
{
    std::optional<std::chrono::system_clock::time_point> expiresAt;
    if (expiry.has_value())
    {
        expiresAt = std::chrono::system_clock::now() + expiry.value();
    }

    ApiKey key(GenerateKey(), description, permission, expiresAt);

    std::lock_guard<std::recursive_mutex> lock(m_Mutex);
    const auto iter = m_ApiKeys.emplace(key.GetKey(), std::move(key));
    return iter.first->first;
}

bool PolicyEngine::RevokeApiKey(const std::string& apiKey)
{
    std::lock_guard<std::recursive_mutex> lock(m_Mutex);
    const auto iter = m_ApiKeys.find(apiKey);
    if(iter != m_ApiKeys.end() && !iter->second.GetIsRevoked())
    {
        iter->second.Revoke();
        return true;
    }

    return false;
}

bool PolicyEngine::ValidateApiKey(const std::string& apiKey, AccessPermission requestedPermission) const
{
    std::lock_guard<std::recursive_mutex> lock(m_Mutex);
    const auto iter = m_ApiKeys.find(apiKey);
    if (iter != m_ApiKeys.end() && !iter->second.GetIsRevoked())
    {
        switch (iter->second.GetPermission())
        {
        case AccessPermission::READ:
            return requestedPermission == AccessPermission::READ || requestedPermission == AccessPermission::READWRITE;

        case AccessPermission::WRITE:
            return requestedPermission == AccessPermission::WRITE || requestedPermission == AccessPermission::READWRITE;

        case AccessPermission::READWRITE:
            return requestedPermission == AccessPermission::READ || requestedPermission == AccessPermission::READWRITE;
        }
    }

    return false;
}

bool PolicyEngine::ValidateApiKey(const std::string& apiKey) const
{
    std::lock_guard<std::recursive_mutex> lock(m_Mutex);
    const auto iter = m_ApiKeys.find(apiKey);
    return iter != m_ApiKeys.end() && !iter->second.GetIsRevoked();
}

size_t PolicyEngine::CleanupExpiredKeys()
{
    size_t removedKeys = 0;

    std::lock_guard<std::recursive_mutex> lock(m_Mutex);
    for (auto iter = m_ApiKeys.begin(); iter != m_ApiKeys.end(); )
    {
        if (IsExpired(iter->second) && (std::chrono::system_clock::now() - iter->second.GetExpiry().value()) > std::chrono::days(30))
        {
            m_ApiKeys.erase(iter);
            ++removedKeys;
        }
        else
        {
            ++iter;
        }
    }

    return removedKeys;
}

std::string PolicyEngine::GenerateKey()
{
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;

    std::stringstream ss;
    ss << "vcpkg_";

    // Generate 32 random hex characters (128 bits of entropy)
    for (int i = 0; i < 4; ++i) 
    {
        uint64_t random_value = dis(gen);
        ss << std::hex << std::setfill('0') << std::setw(16) << random_value;
    }

    return ss.str().substr(0, 38); // "vcpkg_" + 32 hex chars
}

bool PolicyEngine::IsExpired(const ApiKey& key) const
{
    return !key.GetExpiry().has_value() || key.GetExpiry().value() > std::chrono::system_clock::now();
}

bool PolicyEngine::IsExpired(const std::string& apiKey) const
{
    std::lock_guard<std::recursive_mutex> lock(m_Mutex);
    const auto iter = m_ApiKeys.find(apiKey);
    if (iter != m_ApiKeys.end() && !iter->second.GetIsRevoked())
    {
        return IsExpired(iter->second);
    }

    return true;
}

bool PolicyEngine::IsMethodAllowed(AccessPermission permission, const std::string& httpMethod)
{
    std::string lower = httpMethod;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "get")
    {
        if (permission == AccessPermission::READ || permission == AccessPermission::READWRITE)
        {
            return true;
        }
    }
    else if (lower == "post")
    {
        if (permission == AccessPermission::WRITE || permission == AccessPermission::READWRITE)
        {
            return true;
        }
    }
    else if (lower == "head")
    {
        if (permission == AccessPermission::READ || permission == AccessPermission::READWRITE)
        {
            return true;
        }
    }

    return false;
}

std::string ToString(AccessPermission perm)
{
    switch (perm) 
    {
    case AccessPermission::READ: 
        return "read";
    case AccessPermission::WRITE: 
        return "write";
    case AccessPermission::READWRITE: 
        return "readwrite";
    default: 
        return "unknown";
    }
}

std::optional<AccessPermission> FromString(const std::string& str)
{
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "read")
    {
        return AccessPermission::READ;
    }
    if (lower == "write")
    {
        return AccessPermission::WRITE;
    }
    if (lower == "readwrite" || lower == "read-write")
    {
        return AccessPermission::READWRITE;
    }

    return std::nullopt;
}
