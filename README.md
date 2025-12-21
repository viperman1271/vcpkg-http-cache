# vcpkg Binary Cache Server

[![License: GPLv3](https://img.shields.io/badge/License-GPLv3-blue.svg)](./LICENSE)
[![Build](https://github.com/viperman1271/vcpkg-http-cache/actions/workflows/build.yml/badge.svg)](https://github.com/viperman1271/vcpkg-http-cache/actions/workflows/build.yml)

A high-performance HTTP web server for binary caching with vcpkg, built with C++ and Drogon framework.

## License

This project is licensed under the GNU General Public License v3.0.

## Features

- **RESTful API** for binary package management
- **Fast HTTP server** powered by Drogon
- **Efficient storage** with subdirectory organization
- **Package validation** with hash checking
- **Statistics tracking** for uploads/downloads
- **Configurable** via command-line or environment variables
- **Production-ready** with error handling and logging

## Prerequisites

- C++20 compatible compiler (GCC 8+, Clang 7+, MSVC 2022+)
- CMake 3.15 or higher
- vcpkg package manager

## Building

### 1. Install vcpkg (if not already installed)

```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh  # Linux/macOS
# or
.\bootstrap-vcpkg.bat  # Windows
```

### 2. Clone and Build

This assumes usage of vcpkg in manifest mode

```bash
# Clone the repository
git clone https://github.com/viperman1271/vcpkg-http-cache
cd vcpkg-binary-cache-server

# Configure with vcpkg toolchain
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake -S . -B build -G "Visual Studio 17 2022" # Windows
# or
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake -S . -B build # Linux/macOS

# Build
cmake --build build --config Debug -j4
```

## Usage

### Basic Usage

```bash
./bin/vcpkg-binary-cache-server # Linux

# -OR-

.\bin\Release\vcpkg-binary-cache-server.exe # Windows
```

This starts the server on `0.0.0.0:80` with cache directory `/var/vcpkg.cache/cache` (Linux) or `C:\.vcpkg.cache\cache` (Windows).

### Command-Line Options

```bash
./vcpkg-binary-cache-server

OPTIONS:
  -h,     --help              Print this help message and exit
  -c,     --config TEXT       The config file to load (default: /etc/vcpkg.cache/config.toml)
  -s,     --save              Force save the configuration file (if it exists or not). Default:
                              false
  -d,     --daemon            Forces the application to run as a daemon. Default: false # Linux only
```

## API Endpoints

### Authorization/Access Control

If the options have specified the need for an API Key for read, upload, or status endpoints, the vcpkg (or CURL) must include the API Key as a header.

```bash
curl -H "Authorization: Bearer vcpkg_28ea09345eef27c3c93759e530516427" http://localhost/status
```

### Check if Package Exists

```http
HEAD /{triplet}/{name}/{version}/{sha}
```

Returns `200 OK` if package exists, `404 Not Found` if it doesn't.

**Example:**
```bash
curl -I http://localhost/x64-windows/curl/8.17.0/66672cc2e2ace73f33808e213287489112b3a9f1667f0f78b8af05003ebc262f
```

### Download Package

```http
GET /{triplet}/{name}/{version}/{sha}
```

Downloads the binary package with the specified hash.

**Example:**
```bash
curl -O http://localhost/x64-windows/curl/8.17.0/66672cc2e2ace73f33808e213287489112b3a9f1667f0f78b8af05003ebc262f
```

### Upload Package

```http
PUT /{triplet}/{name}/{version}/{sha}
```

Uploads a binary package with the specified hash.

**Example:**
```bash
curl -X PUT --data-binary @package.zip http://localhost/x64-windows/curl/8.17.0/66672cc2e2ace73f33808e213287489112b3a9f1667f0f78b8af05003ebc262f
```

### Server Status

```http
GET /status
```

Returns server statistics and cache information.

**Example:**
```bash
curl http://localhost/status
```

**Response:**
```json
{
  "service": "vcpkg-binary-cache-server",
  "version": "1.0.0",
  "cache_directory": "./cache",
  "package_count": 42,
  "total_size_bytes": 1048576000,
  "total_size_mb": 1000.0,
  "statistics": 
  {
    "total_requests": 150,
    "uploads": 42,
    "downloads": 100
  }
}
```

### Create new API Key

```http
POST /api/keys
```

Create a new API key with the given description and the specified permission

**Example:**
```bash
curl -X POST http://localhost/api/keys -H "Content-Type: application/json" -d `{ \"description\" : \"This is the description of the API key\", \"permission\" : \"readwrite\" }`
```
```powershell
curl -X POST http://localhost/api/keys -H "Content-Type: application/json" -d "{ \"description\" : \"This is the description of the API key\", \"permission\" : \"readwrite\" }"
```

**Response:**
```json
{
    "apiKey": "vcpkg_28ea09345eef27c3c93759e530516427",
    "message": "API key created successfully", 
    "success": true
}
```

## Integrating with vcpkg

To use this server as a binary cache for vcpkg, configure vcpkg with:

```bash
# Set binary cache to use your server
vcpkg install <package> --binarysource="http,http://localhost:8888/{triplet}/{name}/{version}/{sha},readwrite,Authorization: Bearer vcpkg_28ea09345eef27c3c93759e530516427"
```

Or set it in your environment:

```bash
export VCPKG_BINARY_SOURCES="http,http://localhost:8888/{triplet}/{name}/{version}/{sha},readwrite,Authorization: Bearer vcpkg_28ea09345eef27c3c93759e530516427" # Linux
```
## Performance Considerations

- **Thread Pool**: Adjust threads based on your CPU cores
- **File System**: Use a fast filesystem (SSD recommended) for cache directory
- **Network**: Configure appropriate firewall rules for production deployment
- **Memory**: Drogon uses asynchronous I/O, keeping memory usage low

## Security Notes

**Important Security Considerations:**

- This server does NOT include authentication
- By default, authentication is not required. Configure limitations in the options (status, read, write)
- For production use, add authentication middleware or configure internal authentication
- Consider using HTTPS with a reverse proxy (nginx, Apache, Caddy)
- Validate uploaded package integrity
- Implement rate limiting for uploads

## Contributing

Contributions are welcome! Please ensure:
- Code follows C++20 standards
- All endpoints are tested
- Documentation is updated

## Dependencies
- **CLI11**: Command line interface
- **Drogon**: High-performance HTTP framework
- **nlohmann::json**: JSON parsing and generation
- **toml11**: TOML parsing and generation

These are automatically managed by vcpkg through the `vcpkg.json` manifest file.

## Additional Documentation
- [vcpkg Binary Caching](https://learn.microsoft.com/en-us/vcpkg/users/binarycaching)
