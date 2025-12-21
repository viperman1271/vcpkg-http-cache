#include "options.hpp"

#include <fmt/core.h>
#include <toml.hpp>

#include <iostream>

template<typename T>
bool get_toml_value(toml::table& table, const std::string& variable, T& value)
{
    if (table.find(variable) != table.end())
    {
        if constexpr (std::is_same_v<T, std::string>)
        {
            value = table[variable].as_string();
            return true;
        }
        else if constexpr (std::is_same_v<T, int> || std::is_same_v<T, uint32_t> || std::is_same_v<T, uint16_t>)
        {
            value = table[variable].as_integer();
            return true;
        }
        else if constexpr (std::is_same_v<T, bool>)
        {
            value = table[variable].as_boolean();
            return true;
        }
        else if constexpr (std::is_same_v<T, std::chrono::seconds>)
        {
            value = T(table[variable].as_integer());
            return true;
        }
        else if constexpr (std::is_same_v<T, std::vector<std::string>>)
        {
            std::vector<toml::value>& tomlArray = table[variable].as_array();
            value.reserve(tomlArray.size());
            for (toml::value& tomlValue : tomlArray)
            {
                value.emplace_back(std::move(tomlValue.as_string()));
            }
            return true;
        }
        else
        {
            static_assert(!std::is_same_v<T, T>, "Unsupported type for TOML value.");
        }
    }

    return false;
}

#ifdef _WIN32
static const std::string DefaultConfigFile{ "C:\\.vcpkg.cache\\config.toml" };
static const std::string PersistenceFile{ "C:\\.vcpkg.cache\\persistence.json" };
#else
static const std::string DefaultConfigFile{ "/etc/vcpkg.cache/config.toml" };
static const std::string PersistenceFile{ "/etc/vcpkg.cache/persistence.json" };
#endif

Options::Options()
    : configFile{ DefaultConfigFile }
    , persistenceFile{ PersistenceFile }
    , saveConfigFile(false)
#ifndef _WIN32
    , runAsDaemon(false)
#endif // _WIN32
    , sendTermSignal(false)
{
}

void Options::save()
{
    toml::value config;

#ifndef _WIN32
    config["general"]["runAsDaemon"] = runAsDaemon;
#endif // _WIN32

    config["web"]["bind"] = web.bindAddress;
    config["web"]["port"] = web.port;
    config["web"]["threads"] = web.threads;
    config["web"]["logPath"] = web.logPath;
    config["web"]["maxConnectionNum"] = web.maxConnectionNum;
    config["web"]["maxUploadSize"] = web.maxUploadSize;

    config["cache"]["path"] = cache.directory;

    config["upload"]["path"] = upload.directory;

    config["permissions"]["requireAuthForRead"] = permissions.requireAuthForRead;
    config["permissions"]["requireAuthForWrite"] = permissions.requireAuthForWrite;
    config["permissions"]["requireAuthForStatus"] = permissions.requireAuthForStatus;

    try
    {
        std::filesystem::path configFilePath(configFile);
        std::filesystem::create_directories(configFilePath.parent_path());

        std::ofstream file(configFile);
        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open file for writing.");
        }
        file << toml::format(config);
        file.close();
        std::cout << "Config file written successfully: " << configFile << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << " (" << configFile << ")" << std::endl;
    }
}

void Options::load()
{
    if (!std::filesystem::exists(configFile))
    {
        if (saveConfigFile)
        {
            save();
            return;
        }
        else if(configFile != DefaultConfigFile)
        {
            throw std::runtime_error(fmt::format("File \"{}\" does not exist.", configFile));
        }
    }

    toml::value config = toml::parse(configFile);

#ifndef _WIN32
    if (config.contains("general") && config.at("general").is<toml::table>())
    {
        toml::table& generalTable = toml::find<toml::table>(config, "general");
        get_toml_value(generalTable, "runAsDaemon", runAsDaemon);
    }
#endif // _WIN32

    if (config.contains("web") && config.at("web").is<toml::table>())
    {
        toml::table& webTable = toml::find<toml::table>(config, "web");
        get_toml_value(webTable, "bind", web.bindAddress);
        get_toml_value(webTable, "port", web.port);
        get_toml_value(webTable, "threads", web.threads);
        get_toml_value(webTable, "logPath", web.logPath);
        get_toml_value(webTable, "maxConnectionNum", web.maxConnectionNum);
        get_toml_value(webTable, "maxUploadSize", web.maxUploadSize);
    }

    if (config.contains("cache") && config.at("cache").is<toml::table>())
    {
        toml::table& cacheTable = toml::find<toml::table>(config, "cache");
        get_toml_value(cacheTable, "path", cache.directory);
    }

    if (config.contains("upload") && config.at("upload").is<toml::table>())
    {
        toml::table& uploadTable = toml::find<toml::table>(config, "upload");
        get_toml_value(uploadTable, "path", upload.directory);
    }

    if (config.contains("permissions") && config.at("permissions").is<toml::table>())
    {
        toml::table& permissionsTable = toml::find<toml::table>(config, "permissions");
        get_toml_value(permissionsTable, "requireAuthForRead", permissions.requireAuthForRead);
        get_toml_value(permissionsTable, "requireAuthForWrite", permissions.requireAuthForWrite);
        get_toml_value(permissionsTable, "requireAuthForStatus", permissions.requireAuthForStatus);
    }

    if (saveConfigFile)
    {
        save();
    }
}

Options::WebProperties::WebProperties()
    : bindAddress("0.0.0.0")
    , port(80)
    , threads(4)
#ifdef _WIN32
    , logPath("C:\\.vcpkg.cache\\log.txt")
#else
    , logPath("/var/vcpkg.cache/log.txt")
#endif // _WIN32
    , maxConnectionNum(100000)
    , maxUploadSize(1024 * 1024 * 1024) // 1GB
{
}

Options::CacheProperties::CacheProperties()
#ifdef _WIN32
    : directory("C:\\.vcpkg.cache\\cache")
#else
    : directory("/var/vcpkg.cache/cache")
#endif // _WIN32
{
}

Options::UploadProperties::UploadProperties()
#ifdef _WIN32
    : directory("C:\\.vcpkg.cache\\upload")
#else
    : directory("/var/vcpkg.cache/upload")
#endif // _WIN32
{
}

Options::Permissions::Permissions()
    : requireAuthForRead(false)
    , requireAuthForWrite(false)
    , requireAuthForStatus(false)
{

}
