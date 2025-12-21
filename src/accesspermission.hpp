#pragma once

#include <optional>
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
 * @brief Convert AccessPermission to string
 */
std::string ToString(AccessPermission perm);

/**
 * @brief Convert string to AccessPermission
 */
std::optional<AccessPermission> FromString(const std::string& str);