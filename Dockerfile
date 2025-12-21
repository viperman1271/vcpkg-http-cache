# Use an official minimal image Ubuntu as base
FROM ubuntu:latest

# Set the working directory
WORKDIR /bin

# Copy the vcpkg-binary-cache-server binary to /bin in the Docker container
COPY /bin/vcpkg-binary-cache-server /bin/vcpkg-binary-cache-server

# Make sure the /bin/vcpkg-binary-cache-server is executable
RUN chmod +x /bin/vcpkg-binary-cache-server

# Install curl
RUN apt-get update && \
    apt-get install -y curl && \
    rm -rf /var/lib/apt/lists/*

# Set default entry point for the container
ENTRYPOINT ["/bin/vcpkg-binary-cache-server"]
