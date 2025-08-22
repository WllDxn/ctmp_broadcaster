#pragma once
#include <cstdint>
#include <cstddef>
namespace ctmp {

constexpr uint16_t SOURCE_PORT = 33333;
constexpr uint16_t DEST_PORT = 44444;
constexpr uint8_t CTMP_MAGIC = 0xCC;
constexpr uint8_t CTMP_PADDING = 0x00;
// constexpr size_t MAX_MESSAGE_SIZE = 65535;

} // namespace ctmp