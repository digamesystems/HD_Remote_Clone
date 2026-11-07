#pragma once
#include <cstdint>

namespace hd {

// The nRF24 transmits the address MSByte first, but its RX_ADDR/TX_ADDR
// registers (and the RF24 library's byte-array API) take LSByte first.
// Convert an address written in on-air order to register order.
inline void onAirToRegister(const uint8_t* onAir, uint8_t width, uint8_t* reg) {
  for (uint8_t i = 0; i < width; i++) reg[i] = onAir[width - 1 - i];
}

}  // namespace hd
