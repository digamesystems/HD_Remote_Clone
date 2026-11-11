#pragma once
// Hunter Douglas Platinum handheld RF parameters and payload decoding.
//
// Air settings from spitcool/hd-platinum-rf, docs/rf-protocol.md (2026-10-25),
// which cites FCC filings (channels), a community register dump and local SDR
// captures: https://github.com/spitcool/hd-platinum-rf
// Confirmed on both of our remotes by sniffing ch 33 (2026-11-11); the wake
// frames are our own observation. See docs/RF_PROTOCOL.md. These don't apply
// to PowerView/Pebble.

#include <cstddef>
#include <cstdint>
#include <string>

namespace hd {
namespace rf {

// Air settings. The remote hops 33 -> 52 -> 71, one packet per channel.
constexpr uint8_t kChannels[3] = {33, 52, 71};
constexpr uint8_t kAddrWidth = 3;
constexpr uint8_t kAddress[kAddrWidth] = {0xE7, 0xE7, 0xE7};  // nRF24 power-on default
constexpr uint8_t kPayloadSize = 5;
constexpr uint8_t kCrcBytes = 2;  // CRC-16, auto-ack off, static payload
// Data rate: 2 Mbps (hd::Rate::k2M).

// Payload: ID0 ID1 MODE GROUP ACTION. ID0/ID1 identify the remote (the shades
// learn it), so treat them as private.
enum : uint8_t {
  kModeGroup = 0x20,    // group command (arrow pressed in the green window)
  kModeNoGroup = 0x01,  // red-mode (IR) arrow press: the wake frames below

  kGroupNone = 0x00,
  kGroupAll = 0xFF,  // groups 1-4 are one-hot 0x01, 0x02, 0x04, 0x08

  kActionUpHeld = 0xA1,
  kActionDownHeld = 0xA2,
  kActionUpRelease = 0x21,  // release tail; meaning beyond "released" unknown
  kActionDownRelease = 0x22,
  // Red-mode arrow press, either arrow (no direction; the IR carries it).
  // Sent while the key is held. The shade needs it alongside the IR.
  kActionWakeHeld = 0x80,
  // Release tail after kActionWakeHeld. Only one of our two remotes sends it.
  kActionWakeRelease = 0x00,
};

// Our loopback beacon: ID 0000, mode 00, group none, action 55. Mode 00 and
// action 55 have never been seen, so it matches no known command.
constexpr uint8_t kBeaconPayload[kPayloadSize] = {0x00, 0x00, 0x00, 0x00, 0x55};

// The frame a red-mode arrow press sends: ID0 ID1 01 00 80.
void wakePayload(uint8_t id0, uint8_t id1, uint8_t out[kPayloadSize]);

// "1".."4", "All", "none", or "mask 0x03" for anything else.
std::string groupName(uint8_t group);
// "UP held", "DOWN held", "UP release", "DOWN release", "wake held",
// "wake release", or "?".
const char* actionName(uint8_t action);
// e.g. "ID 03C1  mode 20  group 1  UP held". Empty unless len == kPayloadSize.
std::string describePayload(const uint8_t* p, size_t len);

}  // namespace rf
}  // namespace hd
