# Use an official minimal image Ubuntu as base
FROM ubuntu:latest

# Set the working directory
WORKDIR /bin

# Copy the vcpkg-http-cache binary to /bin in the Docker container
COPY /bin/vcpkg-http-cache /bin/vcpkg-http-cache

# Make sure the /bin/vcpkg-http-cache is executable
RUN chmod +x /bin/vcpkg-http-cache

# Install curl
RUN apt-get update && \
    apt-get install -y curl && \
    rm -rf /var/lib/apt/lists/*

# Set default entry point for the container
ENTRYPOINT ["/bin/vcpkg-http-cache"]
