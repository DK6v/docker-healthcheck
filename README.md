# Healthcheck Docker Image

A minimal Docker image containing specialized health check binaries for verifying open TCP/UDP ports on both IPv4 and IPv6 interfaces. It is designed to be used as a multi-stage build source for copying health check tools into application images.

## Image Contents

The image provides four standalone binary utilities:

*   **`TCP`**: Checks for an open TCP port on an IPv4 interface.
*   **`UDP`**: Checks for an open UDP port on an IPv4 interface.
*   **`TCP6`**: Checks for an open TCP port on an IPv6 interface.
*   **`UDP6`**: Checks for an open UDP port on an IPv6 interface.

## How the Tools Work

Each binary reads the corresponding virtual file in the `/proc/net/` directory (e.g., `/proc/net/tcp`, `/proc/net/tcp6`) to determine the listening state of the specified port and address.

**Exit Codes:**
*   **`0` (Success)**: The specified port is open and listening.
*   **`1` (Error)**: The specified port is not found or not in a listening state.

## Usage

### Basic Syntax

```bash
<binary_name> <port_number> [<ip_address>] [--debug] [--quiet]
```

### Arguments

*   `port_number`: **(Required)** The port number to check.
*   `ip_address`: *(Optional)* The specific IP address to check. Defaults to all addresses (`0.0.0.0` for IPv4, `::` for IPv6).
*   `--debug`: *(Optional)* Enables verbose debug output.
*   `--quiet`: *(Optional)* Suppresses all informational output; only the exit code is returned.

### Examples

Check if any IPv4 interface is listening on TCP port 8080:
```bash
TCP 8080
```

Check if the IPv6 loopback (`::1`) is listening on TCP port 2379:
```bash
TCP6 2379 ::1
```

Check for UDP port 53 on a specific IPv4 address with debug output:
```bash
UDP 53 192.168.1.100 --debug
```

## Integration in a Dockerfile

This image is intended to be used in a **multi-stage build**. The binaries are copied from this image into your final application image.

**Example Dockerfile:**
```dockerfile
# Define the version for your main application
ARG APP_VERSION=latest

# Stage 1: Import healthcheck tools
FROM dk6v/healthcheck:latest AS healthcheck

# Stage 2: Build the final application image
FROM your-application-image:${APP_VERSION}

# Copy the healthcheck binaries from the 'healthcheck' stage
COPY --from=healthcheck /healthcheck /healthcheck

# Add the healthcheck directory to the system PATH
ENV PATH="$PATH:/healthcheck"

# ... rest of your Dockerfile configuration ...
```

## Integration in Docker Compose

The copied binaries can then be used directly in your `docker-compose.yaml` file to define a container's health check.

**Example docker-compose.yaml:**
```yaml
services:
  my-app:
    image: my-custom-app
    ports:
      - "80:80"
      - "443:443"
    healthcheck:
      test: >
        TCP 80 &&
        TCP 443
      interval: 10s
      timeout: 5s
      retries: 3
      start_period: 40s
    restart: unless-stopped
```

## Checking Health Status

To inspect the health check status and logs for a running container, use the Docker inspect command:

```bash
# Get detailed health status information
docker inspect --format='{{json .State.Health}}' <container_name> | jq

# Get only the status (healthy/unhealthy)
docker inspect --format='{{.State.Health.Status}}' <container_name>
```

This is useful for debugging health check issues and verifying that your TCP/UDP port checks are working correctly.

## Use Case

This image is perfect for creating robust health checks for services where standard `curl` or `wget` might not be available (e.g., minimal Alpine-based images) or when you need to perform low-level port checks without relying on network tools. It's especially useful for checking IPv6 connectivity and UDP services.
