#include "server.h"
#include "config.h"
#include "client_handler.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdexcept>
#include <iostream>
#include <cstring>

namespace ctmp
{

    Server::Server(uint16_t source_port, uint16_t dest_port)
    {
        // Initialize sockets
        source_socket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (source_socket_ < 0)
        {
            throw std::runtime_error("Failed to create source socket");
        }
        dest_socket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (dest_socket_ < 0)
        {
            throw std::runtime_error("Failed to create destination socket");
        }

        // Socket options
        int opt = 1;
        setsockopt(source_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        setsockopt(dest_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        struct linger lo = { 1, 0 };
        setsockopt(source_socket_, SOL_SOCKET, SO_LINGER, &lo, sizeof(lo));
        // Bind sockets
        sockaddr_in source_addr{};
        source_addr.sin_family = AF_INET;
        source_addr.sin_addr.s_addr = INADDR_ANY;
        source_addr.sin_port = htons(source_port);
        if (bind(source_socket_, (sockaddr *)&source_addr, sizeof(source_addr)) < 0)
        {
            throw std::runtime_error("Source socket bind failed");
        }
        sockaddr_in dest_addr{};
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_addr.s_addr = INADDR_ANY;
        dest_addr.sin_port = htons(dest_port);
        if (bind(dest_socket_, (sockaddr *)&dest_addr, sizeof(dest_addr)) < 0)
        {
            throw std::runtime_error("Destination socket bind failed");
        }

        // Listen on sockets
        listen(source_socket_, 1);
        listen(dest_socket_, SOMAXCONN);
    }

    Server::~Server()
    {
        close(source_socket_);
        close(dest_socket_);
    }

    void Server::start()
    {
        std::cout << "Server started. Source port: " << SOURCE_PORT
                  << ", Destination port: " << DEST_PORT << std::endl;

        while (true)
        {
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(source_socket_, &fds);
            FD_SET(dest_socket_, &fds);

            int max_fd = std::max(source_socket_, dest_socket_);

            // Select for incoming connections
            if (select(max_fd + 1, &fds, nullptr, nullptr, nullptr) < 0)
            {
                throw std::runtime_error("Select error" + std::string(strerror(errno)));
            }

            // Handle source client connection
            if (FD_ISSET(source_socket_, &fds))
            {
                int client_fd = accept(source_socket_, nullptr, nullptr);
                if (client_fd >= 0)
                {
                    handleSourceClient(client_fd);
                }
                else
                {
                    std::cerr << "Failed to accept source client: " << strerror(errno) << std::endl;
                }
            }

            // Handle destination client connections
            if (FD_ISSET(dest_socket_, &fds))
            {
                int client_fd = accept(dest_socket_, nullptr, nullptr);
                if (client_fd >= 0)
                {
                    std::cerr << "New Listener client connected, fd: " << client_fd << std::endl;
                    dest_clients_.push_back(client_fd);
                }
                else
                {
                    std::cerr << "Failed to accept destination client: " << strerror(errno) << std::endl;
                }
            }
            // Clean up disconnected clients
            std::lock_guard<std::mutex> lock(clients_mutex_);
            dest_clients_.erase(
                std::remove_if(dest_clients_.begin(), dest_clients_.end(),
                               [](const auto &fd)
                               {
                                   // Check socket status for destination clients
                                   int error = 0;
                                   socklen_t len = sizeof(error);
                                   return (fd < 0 || getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error != 0);
                               }),
                dest_clients_.end());
            if (source_client_ && !source_client_->isRunning())
            {
                source_client_.reset();
            }
        }
    }

    // Handles a new source client connection on the specified file descriptor.
    void Server::handleSourceClient(int client_fd)
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        // If a source client already exists and is running, refuse connection
        if (source_client_)
        {
            int fd = source_client_->getFd();
            if (source_client_->isRunning())
            {
                std::cerr << "Source client already connected (fd: " << fd << "), rejecting new connection on fd: " << client_fd << std::endl;
                close(client_fd);
                return;
            }
            source_client_.reset();
        }
        // Create new source client
        try
        {
            source_client_ = std::make_unique<ClientHandler>(
                client_fd,
                [this](const CTMPMessage &msg)
                { broadcastMessage(msg); });
            source_client_->start();
            std::cerr << "New source client connected, fd: " << client_fd << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Failed to create source client handler for fd " << client_fd << ": " << e.what() << std::endl;
            close(client_fd);
        }
    }

    // Send valid messages to receiver clients
    void Server::broadcastMessage(const CTMPMessage &msg)
    {
        if (!msg.isValid)
        {
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
        for (auto &client_fd : dest_clients_)
        {
            ssize_t bytes_sent = write(client_fd, buffer.data(), buffer.size());
            if (bytes_sent < 0)
            {
                std::cerr << "Failed to send to client on fd " << client_fd << ": " << strerror(errno) << std::endl;
            }else {
                std::cout << "CTMPMessage sent, length: " << buffer.size() << '\n';
            }
        }
    }
} // namespace ctmp