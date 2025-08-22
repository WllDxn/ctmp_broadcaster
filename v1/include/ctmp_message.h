#pragma once

#include <cstdint>
#include <vector>

namespace ctmp {

    struct CTMPMessage {
        uint8_t magic = 0;
        uint8_t padding = 0;
        uint16_t length = 0;
        std::vector<uint8_t> data;
        bool isValid = false;
    };
    CTMPMessage parseMessage(const std::vector<uint8_t>& buffer);

}