#include "options.hpp"

#include "log.hpp"

#include <libservice/servicerepository.hpp>
#include <toml.hpp>

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
    , configFile{ "C:\\.vcpkg.cache\\config.toml" }
#else
    , configFile{ "/etc/vcpkg.cache/config.toml" }
#endif
{
}

void options_t::init()
{
    load();
}

void options_t::save()
{
    toml::value config;

    config["web"]["bind"] = web.bindAddress;
    config["web"]["port"] = web.port;

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
        log::info() << "Config file written successfully: " << configFile << std::endl;
    }
    catch (const std::exception& e)
    {
        log::error() << "Error: " << e.what() << " (" << configFile << ")" << std::endl;
    }
}

void options_t::load()
{
    if (!std::filesystem::exists(configFile))
    {
        log::error() << "File \"" << configFile << "\" does not exist" << std::endl;
        save();
        return;
    }

    toml::value config = toml::parse(configFile);

    if (config.contains("web") && config.at("web").is<toml::table>())
    {
        toml::table& webTable = toml::find<toml::table>(config, "web");
        get_toml_value(webTable, "bind", web.bindAddress);
        get_toml_value(webTable, "port", web.port);
    }
}

options_t::web_properties::web_properties()
    : bindAddress("0.0.0.0")
    , port(80)
    , threads(4)
{

}
