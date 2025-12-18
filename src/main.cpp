#include "options.hpp"
#include "server.hpp"

#include <drogon/drogon.h>

#include <fstream>
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) 
{
    Options options;
    options.load();

    std::cout << "===========================================" << std::endl
              << "vcpkg Binary Cache Server v1.0.0" << std::endl
              << "===========================================" << std::endl
              << "Configuration:" << std::endl
              << "  Cache Directory: " << options.cache.directory << "" << std::endl
              << "  Host:            " << options.web.bindAddress << "" << std::endl
              << "  Port:            " << options.web.port << "" << std::endl
              << "  Threads:         " << options.web.threads << "" << std::endl
              << "===========================================" << std::endl << std::endl;

    try
    {
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    try 
    {
        // Cache directory
        {
            const std::filesystem::path cachePath(options.cache.directory);
            if (!std::filesystem::exists(cachePath))
            {
                std::filesystem::create_directories(cachePath);
                std::cout << "Creating " << cachePath.string() << std::endl;
            }
        }

        // Log path
        {
            const std::filesystem::path logPath(options.web.logPath);
            if (!std::filesystem::exists(logPath.parent_path()))
            {
                std::cout << "Creating " << logPath.parent_path() << std::endl;
                std::filesystem::create_directories(logPath.parent_path());
            }

            if (!std::filesystem::exists(logPath))
            {
                std::cout << "Creating " << logPath.string() << std::endl;
                std::ofstream logFile(logPath);
                if (logFile)
                {
                    logFile << "";
                }
                logFile.close();
            }
        }

        // Upload directory
        {
            const std::filesystem::path uploadPath(options.upload.directory);
            if (!std::filesystem::exists(uploadPath))
            {
                std::cout << "Creating " << uploadPath.string() << std::endl;
                std::filesystem::create_directories(uploadPath);
            }
        }

        std::shared_ptr<BinaryCacheServer> server = std::make_shared<BinaryCacheServer>(options.cache.directory);
        drogon::app().registerController(server);

        // Configure Drogon
        drogon::app()
            .setLogPath(options.web.logPath)
            .setLogLevel(trantor::Logger::kInfo)
            .addListener(options.web.bindAddress, options.web.port)
            .setThreadNum(options.web.threads)
#ifndef _WIN32
            .enableRunAsDaemon()
#endif // _WIN32
            .setMaxConnectionNum(100000)
            .setMaxConnectionNumPerIP(0)
            .setUploadPath(options.upload.directory)
            .setClientMaxBodySize(1024 * 1024 * 1024); // 1GB max upload

        std::cout << "Starting server on " << options.web.bindAddress << ":" << options.web.port << std::endl;
        std::cout << "Press Ctrl+C to stop the server" << std::endl << std::endl;
        std::cout << "API Endpoints:" << std::endl;
        std::cout << "  HEAD http://" << options.web.bindAddress << ":" << options.web.port << "/{triplet}/{name}/{version}/{sha}  - Check package" << std::endl;
        std::cout << "  GET  http://" << options.web.bindAddress << ":" << options.web.port << "/{triplet}/{name}/{version}/{sha}  - Download package" << std::endl;
        std::cout << "  PUT  http://" << options.web.bindAddress << ":" << options.web.port << "/{triplet}/{name}/{version}/{sha}  - Upload package" << std::endl;
        std::cout << "  GET  http://" << options.web.bindAddress << ":" << options.web.port << "/status  - Server status" << std::endl << std::endl;

        // Run the server
        drogon::app().run();
    } 
    catch (const std::exception& e) 
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}