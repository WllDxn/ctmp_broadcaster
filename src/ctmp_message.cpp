#include "ctmp_message.h"
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <vector>
#include "config.h"

namespace ctmp
{

    class MsgFormatException : public std::runtime_error
    {
    public:
        explicit MsgFormatException(const std::string &msg)
            : std::runtime_error(msg) {}
    };


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
        uint16_t cksum = !sensitive ? msg.checksum : validate_checksum(buffer);
        msg.isValid = true;
        if (msg.magic != CTMP_MAGIC)
        {
            msg.isValid = false;
        }
        msg.isValid = (msg.isValid && (cksum == msg.checksum));
        return msg;
    }

} // namespace ctmp