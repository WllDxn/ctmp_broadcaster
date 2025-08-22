            ██╗    ██╗██╗██████╗ ███████╗    ███████╗████████╗ ██████╗ ██████╗ ███╗   ███╗
            ██║    ██║██║██╔══██╗██╔════╝    ██╔════╝╚══██╔══╝██╔═══██╗██╔══██╗████╗ ████║    ██╗
            ██║ █╗ ██║██║██████╔╝█████╗      ███████╗   ██║   ██║   ██║██████╔╝██╔████╔██║    ╚═╝
            ██║███╗██║██║██╔══██╗██╔══╝      ╚════██║   ██║   ██║   ██║██╔══██╗██║╚██╔╝██║    ██╗
            ╚███╔███╔╝██║██║  ██║███████╗    ███████║   ██║   ╚██████╔╝██║  ██║██║ ╚═╝ ██║    ╚═╝
             ╚══╝╚══╝ ╚═╝╚═╝  ╚═╝╚══════╝    ╚══════╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝╚═╝     ╚═╝

           ______     ______     __         ______     ______     _____     ______     _____
          /\  == \   /\  ___\   /\ \       /\  __ \   /\  __ \   /\  __-.  /\  ___\   /\  __-.
          \ \  __<   \ \  __\   \ \ \____  \ \ \/\ \  \ \  __ \  \ \ \/\ \ \ \  __\   \ \ \/\ \
           \ \_\ \_\  \ \_____\  \ \_____\  \ \_____\  \ \_\ \_\  \ \____-  \ \_____\  \ \____-
            \/_/ /_/   \/_____/   \/_____/   \/_____/   \/_/\/_/   \/____/   \/_____/   \/____/

# CTMP Broadcaster

A simple C++ server that broadcasts CTMP (Custom Transmission Message Protocol) messages from a single source client (port 33333) to multiple destination clients (port 44444). The server ensures only one source client is connected at a time, rejecting additional source connections, and efficiently distributes valid CTMP messages to all connected destination clients.

## Features

- Supports a single source client sending CTMP messages.
- Broadcasts messages to multiple destination clients.
- Thread-safe handling of client connections using mutexes.
- Validates CTMP message format (8-byte header + variable-length data).
- Cleans up disconnected clients to prevent resource leaks.
- Supports validation via checksum

## Prerequisites (local build)

- C++17 or later
- CMake 3.10 or later
- A POSIX-compliant system (e.g., Linux, macOS)
- Standard C++ library

## Build Instructions
### Local Build
1. Clone the repository:
   ```bash
   git clone https://github.com/WllDxn/ctmp_broadcaster.git
   cd ctmp_broadcaster
   ```
2. Create a build directory:
   ```bash
   mkdir build && cd build
   ```
3. Run CMake to configure the project:
   ```bash
   cmake ..
   ```
4. Build the project:
   ```bash
   make
   ```
5. The executable `ctmp_broadcaster` will be generated in the `build` directory.

### Docker Build
1. Clone the repository:
   ```bash
   git clone https://github.com/WllDxn/ctmp_broadcaster.git
   cd ctmp_broadcaster
   ```
2. Build the Docker image:
   ```bash
   docker build -t ctmp_broadcaster .
   ```


## Usage

1. Run the server:

   ```bash
   ./ctmp_broadcaster
   ```
   or
   ```bash
   docker run -p 33333:33333 -p 44444:44444 ctmp_broadcaster
    ```

2. Connect a source client to port 33333 (e.g., using `netcat`):

   ```bash
   nc 127.0.0.1 33333
   ```

3. Connect destination clients to port 44444:

   ```bash
   nc 127.0.0.1 44444
   ```

4. Send CTMP messages from the source client (format: 1 byte magic `0xFF`, 1 byte padding `0x00`, 2 bytes length, 4 bytes padding, variable-length data).

5. Destination clients will receive broadcast messages.

                        ███╗   ███╗██╗███████╗███████╗██╗ ██████╗ ███╗   ██╗
                        ████╗ ████║██║██╔════╝██╔════╝██║██╔═══██╗████╗  ██║
                        ██╔████╔██║██║███████╗███████╗██║██║   ██║██╔██╗ ██║
                        ██║╚██╔╝██║██║╚════██║╚════██║██║██║   ██║██║╚██╗██║
                        ██║ ╚═╝ ██║██║███████║███████║██║╚██████╔╝██║ ╚████║
                        ╚═╝     ╚═╝╚═╝╚══════╝╚══════╝╚═╝ ╚═════╝ ╚═╝  ╚═══╝

                 ██████╗ ██████╗ ███╗   ███╗██████╗ ██╗     ███████╗████████╗███████╗
                ██╔════╝██╔═══██╗████╗ ████║██╔══██╗██║     ██╔════╝╚══██╔══╝██╔════╝
                ██║     ██║   ██║██╔████╔██║██████╔╝██║     █████╗     ██║   █████╗
                ██║     ██║   ██║██║╚██╔╝██║██╔═══╝ ██║     ██╔══╝     ██║   ██╔══╝
                ╚██████╗╚██████╔╝██║ ╚═╝ ██║██║     ███████╗███████╗   ██║   ███████╗
                 ╚═════╝ ╚═════╝ ╚═╝     ╚═╝╚═╝     ╚══════╝╚══════╝   ╚═╝   ╚══════╝
