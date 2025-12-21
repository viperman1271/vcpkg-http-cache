#include <options.hpp>
#include <server.hpp>
#include <filters/authfilter.hpp>

#include <CLI/CLI.hpp>
#include <curl/curl.h>
#include <fmt/core.h>
#include <drogon/drogon.h>

#include <fstream>
#include <iostream>
#include <cstdlib>

// Callback function to handle response data
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) 
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

int main(int argc, char* argv[]) 
{
    CLI::App app{ "vcpkg-binary-cache-server" };

    try 
    {
        std::cout << "===========================================" << std::endl
                    << "vcpkg Binary Cache Server v1.0.0" << std::endl
                    << "===========================================" << std::endl;
        Options options;

#ifdef _WIN32
        app.allow_windows_style_options();
#endif // _WIN32

        app.add_option("-c,--config", options.configFile, fmt::format("The config file to load (default: {})", options.configFile));
        app.add_flag("-s,--save", options.saveConfigFile, "Force save the configuration file (if it exists or not). Default: false");
#ifndef _WIN32
        app.add_flag("-d,--daemon", options.runAsDaemon, "Forces the application to run as a daemon. Default: false");
#endif // _WIN32
        app.add_flag("-k,--kill", options.sendTermSignal, "Sends kill signal via IPC to other instances on the same machine. Default: false");

        app.parse(argc, argv);

        options.load();

        std::cout << "Configuration:" << std::endl
            << "  Cache Directory: " << options.cache.directory << "" << std::endl
            << "  Host:            " << options.web.bindAddress << "" << std::endl
            << "  Port:            " << options.web.port << "" << std::endl
            << "  Threads:         " << options.web.threads << "" << std::endl
            << "===========================================" << std::endl << std::endl;

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

        if (options.sendTermSignal)
        {
            CURL* curl;
            std::string readBuffer;

            curl_global_init(CURL_GLOBAL_DEFAULT);
            curl = curl_easy_init();

            curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:8888/internal/kill");
            // Set the HTTP method to POST (you can change to GET if needed)
            //curl_easy_setopt(curl, CURLOPT_GET, 1L);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

            std::cout << "Sending kill signal to local instances." << std::endl;
            const CURLcode res = curl_easy_perform(curl);

            if (res != CURLE_OK)
            {
                std::cerr << "ERR: No local instances appear to be running." << std::endl;
            }
            else
            {
                long response_code;
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            }

            curl_easy_cleanup(curl);

            return 0;
        }
        
        std::shared_ptr<BinaryCacheServer> server = std::make_shared<BinaryCacheServer>(options.cache.directory, options.persistenceFile);
        drogon::app().registerController(server);

        std::shared_ptr<ApiKeyFilter> filter = server->CreateApiKeyFilter(options.permissions.requireAuthForRead, options.permissions.requireAuthForWrite, options.permissions.requireAuthForStatus);
        drogon::app().registerFilter(filter);

        // Configure Drogon
        drogon::app()
            .setLogPath(options.web.logPath)
            .setLogLevel(trantor::Logger::kInfo)
            .addListener(options.web.bindAddress, options.web.port)
            .setThreadNum(options.web.threads)
            .setMaxConnectionNum(options.web.maxConnectionNum)
            .setMaxConnectionNumPerIP(0)
            .setUploadPath(options.upload.directory)
            .setClientMaxBodySize(options.web.maxUploadSize);

#ifndef _WIN32
        if (options.runAsDaemon)
        {
            std::cout << "Running application as a daemon" << std::endl;
            drogon::app()
                .enableRunAsDaemon();
        }
#endif // _WIN32

        std::cout << "Starting server on " << options.web.bindAddress << ":" << options.web.port << std::endl;
        std::cout << "Press Ctrl+C to stop the server" << std::endl << std::endl;
        std::cout << "API Endpoints:" << std::endl;
        std::cout << "  HEAD   http://" << options.web.bindAddress << ":" << options.web.port << "/{triplet}/{name}/{version}/{sha}  - Check package" << std::endl;
        std::cout << "  GET    http://" << options.web.bindAddress << ":" << options.web.port << "/{triplet}/{name}/{version}/{sha}  - Download package" << std::endl;
        std::cout << "  PUT    http://" << options.web.bindAddress << ":" << options.web.port << "/{triplet}/{name}/{version}/{sha}  - Upload package" << std::endl;
        std::cout << "  GET    http://" << options.web.bindAddress << ":" << options.web.port << "/status  - Server status" << std::endl;
        std::cout << "  POST   http://localhost:" << options.web.port << "/api/keys  - Create new API key" << std::endl;
        std::cout << "  GET    http://localhost:" << options.web.port << "/api/keys/{key} - Get API key info" << std::endl;
        std::cout << "  DELETE http://localhost:" << options.web.port << "/api/keys/{key} - Revokes/invalidates specified key" << std::endl;
        std::cout << "  POST   http://localhost:" << options.web.port << "/api/keys/cleanup - Will execute cleanup of expired keys" << std::endl;
        std::cout << std::endl;

        // Run the server
        drogon::app().run();
    } 
    catch (const CLI::ParseError& e)
    {
        return app.exit(e);
    }
    catch (const std::exception& e) 
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}