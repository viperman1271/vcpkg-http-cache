cmake_minimum_required(VERSION 3.20)
project(vcpkg-binary-cache-server)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find required packages
find_package(Drogon CONFIG REQUIRED)
find_package(jsoncpp CONFIG REQUIRED)
find_package(toml11 CONFIG REQUIRED)

source_group("" FILES
    src/main.cpp
    src/options.cpp
    src/options.hpp
    src/server.cpp
    src/server.hpp
)

add_executable(vcpkg-binary-cache-server
    src/main.cpp
    src/options.cpp
    src/options.hpp
    src/server.cpp
    src/server.hpp
)

target_include_directories(vcpkg-binary-cache-server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(vcpkg-binary-cache-server PUBLIC
    Drogon::Drogon
    jsoncpp_lib
    toml11::toml11
)