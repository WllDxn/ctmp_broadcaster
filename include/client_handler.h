#pragma once

#include <atomic>
#include <functional>
#include <thread>
#include "ctmp_message.h"

namespace ctmp {

class ClientHandler {
public:
    ClientHandler(int fd, std::function<void(const CTMPMessage&)> broadcast_cb);
    ~ClientHandler();
    void start();
    void stop();
    bool isRunning() const;
    void send(const std::vector<uint8_t>& data);
    int getFd() const { return fd_; }
private:
    int fd_;
    std::atomic<bool> running_;
    std::function<void(const CTMPMessage&)> broadcast_callback_;
    std::thread thread_;
    void handleMessages();
};

} // namespace ctmp