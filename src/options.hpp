#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct Options final
{
public:
    Options();
    ~Options() = default;

    std::string configFile;
    bool saveConfigFile;

    struct WebProperties
    {
        WebProperties();

        std::string bindAddress;
        uint16_t port;
        uint16_t threads;
        std::string logPath;
        uint32_t maxConnectionNum;
        uint32_t maxUploadSize;
    } web;

    struct CacheProperties
    {
        CacheProperties();

        std::string directory;
    } cache;

    struct UploadProperties
    {
        UploadProperties();

        std::string directory;
    } upload;

    void save();
    void load();
};