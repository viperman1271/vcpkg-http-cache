# vcpkg Binary Cache Server

A high-performance HTTP web server for binary caching with vcpkg, built with C++ and Drogon framework.

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
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake -S . -G "Visual Studio 17 2022" # Windows
# or
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake -S . # Linux/macOS

# Build
cmake --build build --config Debug -j4
```

## Usage

### Basic Usage

```bash
./build/bin/vcpkg-binary-cache-server
```

This starts the server on `0.0.0.0:80` with cache directory `./cache`.

### Command-Line Options

```bash
./vcpkg-binary-cache-server
```

## API Endpoints

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
  "statistics": {
    "total_requests": 150,
    "uploads": 42,
    "downloads": 100
  }
}
```

## Integrating with vcpkg

To use this server as a binary cache for vcpkg, configure vcpkg with:

```bash
# Set binary cache to use your server
vcpkg install <package> --binarysource=http,http://localhost/{triplet}/{name}/{version}/{sha},readwrite
```

Or set it in your environment:

```bash
export VCPKG_BINARY_SOURCES="http,http://localhost/{triplet}/{name}/{version}/{sha},readwrite" # Linux
```

## Project Structure

```
vcpkg-binary-cache-server/
├── CMakeLists.txt           # CMake build configuration
├── vcpkg.json              # vcpkg manifest with dependencies
├── README.md               # This file
├── include/
│   └── server.hpp          # Server class header
└── src/
    ├── main.cpp            # Application entry point
    └── server.cpp          # Server implementation
```

## Storage Organization

Packages are stored with subdirectory sharding to prevent filesystem issues with large numbers of files:

```
cache/
├── ab/
│   └── abc123def456.zip
├── cd/
│   └── cdef789012345.zip
└── ...
```

The first two characters of the hash determine the subdirectory.

## Performance Considerations

- **Thread Pool**: Adjust `--threads` based on your CPU cores
- **File System**: Use a fast filesystem (SSD recommended) for cache directory
- **Network**: Configure appropriate firewall rules for production deployment
- **Memory**: Drogon uses asynchronous I/O, keeping memory usage low

## Security Notes

⚠️ **Important Security Considerations:**

- This server does NOT include authentication
- Suitable for trusted networks only
- For production use, add authentication middleware
- Consider using HTTPS with a reverse proxy (nginx, Apache, Caddy)
- Validate uploaded package integrity
- Implement rate limiting for uploads

## Troubleshooting

### Port Already in Use

```bash
# Change port
./vcpkg-binary-cache-server --port 9000
```

### Permission Denied on Cache Directory

```bash
# Ensure directory is writable
mkdir -p /path/to/cache
chmod 755 /path/to/cache
./vcpkg-binary-cache-server --cache-dir /path/to/cache
```

### Build Errors

```bash
# Make sure vcpkg toolchain is specified
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake

# Clean and rebuild
rm -rf build
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

## License

This project is provided as-is for use with vcpkg binary caching.

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

https://learn.microsoft.com/en-us/vcpkg/users/binarycaching