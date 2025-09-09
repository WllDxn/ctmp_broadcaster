# Filestructure
```
wirestorm/
├── include/
│   ├── client_handler.h
│   ├── config.h
│   ├── ctmp_message.h
│   └── server.h
└── src/
    ├── client_handler.cpp
    ├── client_message.cpp
    ├── main.cpp
    └── server.cpp
```
# Project components
## main
```cpp
#include <iostream>
#include "server.h"
#include "config.h"
using namespace ctmp;

int main() {
    try {
        ctmp::Server server(ctmp::SOURCE_PORT, ctmp::DEST_PORT);
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
```
## server
### Constructor & destructor
```cpp
namespace ctmp {

Server::Server(uint16_t source_port, uint16_t dest_port) {
    // Initialize sockets
    source_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (source_socket_ < 0) {
        throw std::runtime_error("Failed to create source socket");
    }
    dest_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (dest_socket_ < 0) {
        throw std::runtime_error("Failed to create destination socket");
    }

    // Socket options
    int opt = 1;
    setsockopt(source_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(dest_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Bind sockets
    sockaddr_in source_addr{};
    source_addr.sin_family = AF_INET;
    source_addr.sin_addr.s_addr = INADDR_ANY;
    source_addr.sin_port = htons(source_port);
    if (bind(source_socket_, (sockaddr*)&source_addr, sizeof(source_addr)) < 0) {
        throw std::runtime_error("Source socket bind failed");
    }
    sockaddr_in dest_addr{};
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = INADDR_ANY;
    dest_addr.sin_port = htons(dest_port);
    if (bind(dest_socket_, (sockaddr*)&dest_addr, sizeof(dest_addr)) < 0) {
        throw std::runtime_error("Destination socket bind failed");
    }

    // Listen on sockets
    listen(source_socket_, 1); 
    listen(dest_socket_, SOMAXCONN);
}

Server::~Server() {
    close(source_socket_);
    close(dest_socket_);
}
```
Initialise socket params
 - AF_INET - Use IPv4
 - SOCK_STREAM - Stream-oriented socket
 - 0 - Select protocol automatically (TCP)

Set sockets to reuse address
 - SOL_SOCKET - Select socket layer
 - SO_REUSEADDR - Reuse address
 - opt - Set to 1/True

This is so that the server can be restarted quickly by allowing sockets to bind to a port in TIME_WAIT state, and so that multiple listeners can connect to port 44444.

Bind sockets
 - INADDR_ANY - Binds socket to all available network interfaces, allowing connections from any IP
 - htons(port) - sets the port in network byte order (host-to-network short)

Listen
 - 1 - enforce single connection on source_socket_
 - SOMAXCONN - Allow multiple connections up to maximum queue length specifiable by listen for dest_socket_

### start()
```cpp
void Server::start() {
    std::cout << "Server started. Source port: " << SOURCE_PORT 
              << ", Destination port: " << DEST_PORT << std::endl;

    while (true) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(source_socket_, &fds);
        FD_SET(dest_socket_, &fds);

        int max_fd = std::max(source_socket_, dest_socket_);
        
        // Select for incoming connections
        if (select(max_fd + 1, &fds, nullptr, nullptr, nullptr) < 0) {
            throw std::runtime_error("Select error" + std::string(strerror(errno) ));
        }

        // Handle source client connection
        if (FD_ISSET(source_socket_, &fds)) {
            int client_fd = accept(source_socket_, nullptr, nullptr);
            std::cout << "Source client connected on socket: " << client_fd << '\n';
            if (client_fd >= 0) {
                handleSourceClient(client_fd);
            }else {
                std::cerr << "Failed to accept source client: " << strerror(errno) << std::endl;
            }
        }

        // Handle destination client connections
        if (FD_ISSET(dest_socket_, &fds)) {
            int client_fd = accept(dest_socket_, nullptr, nullptr);
            if (client_fd >= 0) {
                auto handler = std::make_unique<ClientHandler>(
                    client_fd, 
                    [this](const CTMPMessage& msg) { broadcastMessage(msg); }
                );
                dest_clients_.push_back(std::move(handler));
            }else {
                std::cerr << "Failed to accept destination client: " << strerror(errno) << std::endl;
            }
        }
        // Clean up disconnected clients
        std::lock_guard<std::mutex> lock(clients_mutex_);
        dest_clients_.erase(
            std::remove_if(dest_clients_.begin(), dest_clients_.end(),
                [](const auto& client) {
                        // Check socket status for destination clients
                        int fd = client->getFd();
                        int error = 0;
                        socklen_t len = sizeof(error);
                        return (fd < 0 || getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error != 0);

                    }),
            dest_clients_.end()
        );
        if (source_client_ && !source_client_->isRunning()) {
            source_client_.reset();
        }
    }
}
```
Monitor incoming connection
 - fd_set - Structure defined in select.h, capable of monitoring multiple file descriptors.
 - FD_ZERO - sets fds to empty state.
 - FD_SET - Adds source_socket_ and dest_socket_ to fd_set for monitoring

This allows the server to monitor multiple sockets on a single thread.

Select for incoming connections
 - max_fd - Find the highest file descriptor
 - select - blocks until at least one socket is readable. Then sets it's file descriptor to 1, indicating it can be read.
   - max_fds+1 - Checks file descriptors 0 to max_fds
   - fds - Specify file descriptor set
   - nullptr*3 - No write or error sets monitored, no timeout is specified
 - Throws exception if signal interrupted or invalid socket

Source client connection
 - FD_ISSET - checks source_socket_ in fds to see if it is readable.
 - accept - creates new file descriptor for client.
 - if (client_fd >= 0) - checks valid file descriptor
    - handleSourceClient - Calls function to handle source client connection

Destination client connection
 - std::make_unique\<ClientHandler> - Constructs a new ClientHandler, and wraps it in a unique pointer.
   - Ensures each new ClientHandler is properly allocated and deleted.
 - Parameters for ClientHandler:
    - client_fd - file descriptor of this client connection.
    - Lambda function - Useless for destination clients

Clean up disconnected clients
 - lock_guard - Locks thread (unnecessary as destination_clients are single threaded)
 - remove_if - Removes any clients in dest_clients that have disconnected.
   - getsockopt - Put the current value for socket FD's SO_ERROR option into OPTVAL (error)
 - If source client exists and self-reports false for isRunning(), reset.

### handleSourceClient()
```cpp
void Server::handleSourceClient(int client_fd) {    
    std::lock_guard<std::mutex> lock(clients_mutex_);
    if (source_client_ && source_client_->isRunning()) {
        std::cerr << "Source client already connected, rejecting new connection on fd: " << client_fd << std::endl;
        close(client_fd);
        return;
    }
    source_client_ = std::make_unique<ClientHandler>(
        client_fd, 
        [this](const CTMPMessage& msg) { broadcastMessage(msg); }
    );
    source_client_->start();
}
```
 - Ensure thread safety across separate Clienthandler::start() threads
 - If a source client already exists and is running, refuse connection
 - Create new source client
    - client_fd - file descriptor of this client connection.
    - Lambda function - Captures server object and calls Server::broadcastMessage to use in callback function.
 - Calls start() function, detaching thread.

### broadcastMessage()
```cpp
void Server::broadcastMessage(const CTMPMessage& msg) {
    if (!msg.isValid) {
        std::cerr << "Invalid CTMP message dropped" << std::endl;
        return;
    }

    std::vector<uint8_t> buffer;
    buffer.push_back(msg.magic);
    buffer.push_back(msg.options);
    buffer.push_back(static_cast<uint8_t>(msg.length >> 8));
    buffer.push_back(static_cast<uint8_t>(msg.length & 0xFF));
    buffer.push_back(static_cast<uint8_t>(msg.checksum >> 8));
    buffer.push_back(static_cast<uint8_t>(msg.checksum & 0xFF));
    buffer.resize(buffer.size() + 2, CTMP_PADDING);
    buffer.insert(buffer.end(), msg.data.begin(), msg.data.end());

    std::lock_guard<std::mutex> lock(clients_mutex_);
    for (auto& client : dest_clients_) {
        client->send(buffer);
        std::cout << "CTMPMessage sent, length: "<<buffer.size() << '\n';
    }
}
```
 - Check if message is valid
 - Reconstruct message in original format
 - Send message to all clients.

## client_handler
### Constructor & utility functions
```cpp
ClientHandler::ClientHandler(int fd, std::function<void(const CTMPMessage &)> broadcast_cb) : fd_(fd), running_(true), broadcast_callback_(broadcast_cb) {}

    ClientHandler::~ClientHandler()
    {
        stop();
        close(fd_);
    }

    void ClientHandler::start()
    {
        std::thread([this](){ handleMessages(); })
            .detach();
    }

    void ClientHandler::stop()
    {
        running_ = false;
    }

    // Only used for source clients
    bool ClientHandler::isRunning() const
    {
        return running_;
    }
```
#### ClientHandler::start()
 - Runs handleMessages() in a detached thread, allowing parsing of messages to be performed without blocking connection of clients.
 - When handleMessages() exits (via disconnection), it sets running_ to false and terminates the thread, but does not deconstruct the ClientHandler, allowing the Server to deal with it.
### handleMessages()
```cpp
void ClientHandler::handleMessages()
    {
        std::vector<uint8_t> buffer;
        uint8_t temp_buffer[1024] = {};

        while (running_)
        {
            ssize_t bytes_read = read(fd_, temp_buffer, sizeof(temp_buffer));
            if (bytes_read <= 0)
            {
                std::cerr << "Read error or client disconnected on fd " << fd_ << std::endl;
                running_ = false;
                break;
            }

            buffer.insert(buffer.end(), temp_buffer, temp_buffer + bytes_read);

            while (buffer.size() >= 8)
            {
                try
                {
                    CTMPMessage msg = parseMessage(buffer);
                    if (msg.length + 8 <= buffer.size())
                    {
                        if (msg.isValid)
                        {
                            if (broadcast_callback_)
                            {
                                broadcast_callback_(msg);
                            }
                            else
                            {
                                std::cerr << "Broadcast callback not set for fd " << fd_ << std::endl;
                            }
                        }
                        else
                        {
                            std::cerr << "Invalid CTMP message dropped on fd " << fd_ << std::endl;
                        }
                        buffer.erase(buffer.begin(), buffer.begin() + msg.length + 8);
                    }
                    else
                    {
                        break;
                    }
                }
                catch (const std::runtime_error &e)
                {
                    std::cerr << "Error parsing message on fd " << fd_ << ": " << e.what() << std::endl;
                    buffer.clear();
                    break;
                }
            }
            // Optional: checking buffer size 
            // if (buffer.size() > MAX_MESSAGE_SIZE)
            // {
            //     std::cerr << "Buffer overflow on fd " << fd_ << ", clearing buffer" << std::endl;
            //     buffer.clear();
            // }
        }
    }
```
 - Reads into tempbuffer
 - Inserts tempbuffer on to the end of buffer
 - When buffer contains headers (8 bytes), try to parse the message.
 - Parsing headers gives length, if data received isn't sufficient in length, continue reading.
 - When message is of correct length, test message validity (contained in ctmp_message).
    - If message is valid, trigger callback function, broadcasting message
    - If message is invalid, log error
## ctmp_message
### parseMessage()
```cpp
CTMPMessage parseMessage(const std::vector<uint8_t> &buffer)
    {
        if (buffer.size() < 8)
        {
            throw MsgFormatException("Incomplete CTMP message");
        }
        CTMPMessage msg;
        // Extract data from buffer
        msg.magic = buffer[0];
        msg.options = buffer[1];
        bool sensitive = (buffer[1] & 0b01000000) != 0;
        msg.length = (static_cast<uint16_t>(buffer[2]) << 8) | buffer[3];
        msg.checksum = (static_cast<uint16_t>(buffer[4]) << 8) | buffer[5];
        // Copy data, read to end of buffer or end of message.
        msg.data = std::vector<uint8_t>(buffer.begin() + 8, std::min(buffer.begin() + 8 + msg.length, buffer.end()));
        // Validate checksum
        uint16_t cksum = sensitive ?  validate_checksum(buffer) : msg.checksum ;
        msg.isValid = true;
        if (msg.magic != CTMP_MAGIC)
        {
            msg.isValid = false;
        }
        msg.isValid = (msg.isValid && (cksum == msg.checksum));
        if (sensitive && !(cksum == msg.checksum)){
            std::cerr << "Invalid checksum on sensitive CTMPMessage. Not sending." << '\n';
        }
        return msg;
    }
```
 - Extracts magic char, sensitive bit, message length, checksum and data from received message.
 - Reads min of message length (from headers), or to end of buffer.
 - If message is sensitive, validates checksum.
 - Sets message as invalid if Magic char incorrect or checksum does not pass test.
### validate_checksum()
```cpp
    uint16_t validate_checksum(const std::vector<uint8_t> &data)
    {
        uint32_t sum = 0;

        // Process pairs of bytes to form 16-bit words
        for (size_t i = 0; i + 1 < data.size(); i += 2)
        {
            // Combine two bytes into a 16-bit word (big-endian)
            uint16_t word;
            if (i == 4)
            {
                word = 0xCCCC;
            }
            else
            {

                word = (static_cast<uint16_t>(data[i]) << 8) | data[i + 1];
            }
            sum += word;
            // Handle carry
            while (sum > 0xFFFF)
            {
                sum = (sum & 0xFFFF) + (sum >> 16);
            }
        }

        // Handle odd number of bytes by padding with zero
        if (data.size() % 2 == 1)
        {
            uint16_t word = static_cast<uint16_t>(data.back()) << 8;
            sum += word;

            // Handle carry
            while (sum > 0xFFFF)
            {
                sum = (sum & 0xFFFF) + (sum >> 16);
            }
        }

        return static_cast<uint16_t>(~sum);
    }
```
 - Process pairs of bytes to form 16-bit words
 - Combine two bytes into a 16-bit word (big-endian)
 - Overwrite checksum bytes with 0xCCCC
 - Add value of word to sum
 - If sum > 0xFFFF (65535), it has overflowed 16-bit range.
    - Wrap around overflow by adding carry (sum >> 16) back into lower 16 bits.
 - Handle odd number of bytes by padding with zero.

# Improvements
 - Move write logic for destination clients into server.cpp as this is the only functionality they recquire and therefore do not need to be the same type of object as source clients. 
 - Clean up references to mutex from earlier versions of implementation.