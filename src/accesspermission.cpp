#include <accesspermission.hpp>

#include <algorithm>

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