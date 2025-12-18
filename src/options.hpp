#pragma once

#include <string>
#include <vector>

struct options_t final
{
public:
    options_t();
    ~options_t() = default;

    std::string configFile;

    struct web_properties
    {
        web_properties();

        std::string bindAddress;
        uint16_t port;
        uint16_t threads;
    } web;

    struct cache_properties
    {
        std::string directory;
    } cache;

    void save();
    void load();
};