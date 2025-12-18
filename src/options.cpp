#include "options.hpp"

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

options_t::options_t()
#ifdef _WIN32
    : configFile{ "C:\\.vcpkg.cache\\config.toml" }
#else
    : configFile{ "/etc/vcpkg.cache/config.toml" }
#endif
{
}

void options_t::save()
{
    toml::value config;

    config["web"]["bind"] = web.bindAddress;
    config["web"]["port"] = web.port;
    config["web"]["threads"] = web.threads;
    config["web"]["logPath"] = web.logPath;
    
    config["cache"]["path"] = cache.directory;

    config["upload"]["path"] = upload.directory;

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

void options_t::load()
{
    if (!std::filesystem::exists(configFile))
    {
        std::cerr << "File \"" << configFile << "\" does not exist" << std::endl;
        save();
        return;
    }

    toml::value config = toml::parse(configFile);

    if (config.contains("web") && config.at("web").is<toml::table>())
    {
        toml::table& webTable = toml::find<toml::table>(config, "web");
        get_toml_value(webTable, "bind", web.bindAddress);
        get_toml_value(webTable, "port", web.port);
        get_toml_value(webTable, "threads", web.threads);
        get_toml_value(webTable, "logPath", web.logPath);
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
}

options_t::web_properties::web_properties()
    : bindAddress("0.0.0.0")
    , port(80)
    , threads(4)
#ifdef _WIN32
    , logPath("C:\\.vcpkg.cache\\log.txt")
#else
    , logPath("/var/vcpkg.cache/log.txt")
#endif // _WIN32
{
}

options_t::cache_properties::cache_properties()
#ifdef _WIN32
    : directory("C:\\.vcpkg.cache\\cache")
#else
    : directory("/var/vcpkg.cache/cache")
#endif // _WIN32
{
}

options_t::upload_properties::upload_properties()
#ifdef _WIN32
    : directory("C:\\.vcpkg.cache\\upload")
#else
    : directory("/var/vcpkg.cache/upload")
#endif // _WIN32
{

}
