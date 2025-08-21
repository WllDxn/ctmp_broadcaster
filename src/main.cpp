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