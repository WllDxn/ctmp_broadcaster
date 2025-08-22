#pragma once

#include <cstdint>
#include <vector>

namespace ctmp
{

    struct CTMPMessage
    {
        uint8_t magic = 0;
        uint8_t options = 0;
        uint16_t length = 0;
        uint16_t checksum;
        std::vector<uint8_t> data;
        bool isValid = false;
    };
    CTMPMessage parseMessage(const std::vector<uint8_t> &buffer);
    uint16_t computeChecksum(const std::vector<uint8_t> &data, const bool validate);
    uint16_t validate_checksum(const std::vector<uint8_t>& data) ;
    uint16_t compute_correct_checksum(const std::vector<uint8_t>& data);
}