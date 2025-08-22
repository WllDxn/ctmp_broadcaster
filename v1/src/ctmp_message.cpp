#include "ctmp_message.h"
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include "config.h"

namespace ctmp {

class MsgFormatException : public std::runtime_error {
public:
    explicit MsgFormatException(const std::string &msg)
        : std::runtime_error(msg) {}
};

CTMPMessage parseMessage(const std::vector<uint8_t> &buffer) {
    if (buffer.size() < 8) {
        throw MsgFormatException("Incomplete CTMP message");
    }
    CTMPMessage msg;
    // Extract data from buffer
    msg.magic = buffer[0];
    msg.padding = buffer[1];
    msg.length = (static_cast<uint16_t>(buffer[2]) << 8) | buffer[3];
    
    // Copy data, read to end of buffer or end of message.
    msg.data = std::vector<uint8_t>(buffer.begin() + 8, std::min(buffer.begin() + 8 + msg.length, buffer.end()));
    
    // Validate message format
    msg.isValid = true;
    if (msg.magic != CTMP_MAGIC) {
        msg.isValid = false;
    } else if (msg.padding != CTMP_PADDING) {
        msg.isValid = false;
    } else if (!std::all_of(buffer.begin() + 4, buffer.begin() + 8, [pad = msg.padding](uint8_t b) { return b == pad; })) {
        msg.isValid = false;
    }
    
    return msg;
}

} // namespace ctmp