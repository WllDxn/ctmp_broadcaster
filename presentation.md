# Wirestorm
## Filestructure
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
## Project components
### ctmp_message
<pre>```cpp
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
```</pre>