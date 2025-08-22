#include "client_handler.h"
#include "ctmp_message.h"
#include "config.h"
#include <thread>
#include <stdexcept>
#include <unistd.h>
#include <iostream>
#include <cstring>

namespace ctmp
{
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

    // Sends data to client, called by server object with validated data
    void ClientHandler::send(const std::vector<uint8_t> &data)
    {
        
        if (fd_ >= 0)
        {
            ssize_t bytes_sent = write(fd_, data.data(), data.size());
            if (bytes_sent < 0)
            {
                std::cerr << "Failed to send to client on fd " << fd_ << ": " << strerror(errno) << std::endl;
                running_ = false;
            }
        }
    }

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

} // namespace ctmp