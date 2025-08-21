# Stage 1: Builder
FROM gcc:12 AS builder

# Install CMake and build essentials
RUN apt-get update && apt-get install -y cmake build-essential

# Set working directory
WORKDIR /app

# Copy source files
COPY . .

# Create build directory and compile
RUN mkdir build && cd build && cmake .. && make

# Stage 2: Runtime
FROM ubuntu:22.04

# Install runtime dependencies (minimal)
RUN apt-get update && apt-get install -y libstdc++6 && rm -rf /var/lib/apt/lists/*

# Copy the compiled executable from the builder stage
COPY --from=builder /app/build/ctmp_broadcaster /usr/bin/ctmp_broadcaster

# Expose ports for source (33333) and destination (44444) clients
EXPOSE 33333 44444

# Run the broadcaster
ENTRYPOINT ["/usr/bin/ctmp_broadcaster"]