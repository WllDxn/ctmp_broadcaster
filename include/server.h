#pragma once

#include <vector>
#include <memory>
#include <mutex>
#include "client_handler.h"

namespace ctmp {

class Server {
public:
    Server(uint16_t source_port, uint16_t dest_port);
    ~Server();
    void start();

private:
    int source_socket_;
    int dest_socket_;
    mutable std::mutex clients_mutex_;
    std::vector<std::unique_ptr<ClientHandler>> dest_clients_;
    std::unique_ptr<ClientHandler> source_client_;
    void handleSourceClient(int client_fd);
    void broadcastMessage(const CTMPMessage& msg);
};

} // namespace ctmp