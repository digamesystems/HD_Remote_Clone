// SPDX-License-Identifier: GPL-2.0-or-later
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
  // Setup mode (group assignment, manual p. 5): SETUP, group key, then OPEN
  // (add) or CLOSE (remove) aimed at the shade. Sniffed 2026-11-12 with
  // ACTION 80 while held, then sometimes a 00 tail. The IR is the normal
  // OPEN / CLOSE code; it picks the shade, and this frame gives the ID + group.
  kModeSetupAdd = 0x30,
  kModeSetupRemove = 0x31,
  // Absolute position (experimental). Layout from spitcool/hd-platinum-rf,
  // docs/rf-protocol.md, "Position packets correlated with original-bridge
  // commands - 2026-10-23": the Platinum bridge sends ID0 ID1 50 MASK POS for
  // about 1.49 s, then ID0 ID1 32 MASK POS for about 0.30 s. MASK is one-hot
  // (bridge: 1 << (group_field - 1), up to 0x40); POS is raw 0-255. Our shade
  // test (2026-11-18): they work from a handheld-paired ID; MASK is the
  // handheld group byte (01/02/04/08, bits combine); POS is absolute, 0 and
  // 255 the two limits. A made-up ID gets no response.
  kModePosPhase1 = 0x50,
  kModePosPhase2 = 0x32,

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

// The frame a red-mode arrow press sends: ID0 ID1 01 00 80.
void wakePayload(uint8_t id0, uint8_t id1, uint8_t out[kPayloadSize]);

// A group command frame: ID0 ID1 20 GROUP ACTION. held: A1 (up) / A2 (down),
// sent while the arrow is held; otherwise the release tail 21 / 22. Up/A1/21
// were seen on our remotes; down/A2/22 come from spitcool's notes.
void groupPayload(uint8_t id0, uint8_t id1, uint8_t group, bool up, bool held,
                  uint8_t out[kPayloadSize]);

// A setup-mode (group assignment) frame: ID0 ID1 30|31 GROUP 80|00.
// add: 30 (OPEN), else 31 (CLOSE). held: 80, else the 00 release tail.
void setupPayload(uint8_t id0, uint8_t id1, uint8_t group, bool add, bool held,
                  uint8_t out[kPayloadSize]);

// Bridge-timed phase lengths for position frames, and the cap per phase.
constexpr uint32_t kPosPhase1Ms = 1490;
constexpr uint32_t kPosPhase2Ms = 300;
constexpr uint32_t kPosMaxPhaseMs = 2000;

// A position frame (experimental): ID0 ID1 50|32 MASK POS. phase1: 50, else 32.
void positionPayload(uint8_t id0, uint8_t id1, uint8_t mask, uint8_t pos, bool phase1,
                     uint8_t out[kPayloadSize]);

// Position-command mask: 1 or 2 hex digits (optional "0x"), nonzero. Any
// nonzero byte is accepted (the bridge's are one-hot, up to 0x40). -1 if invalid.
int parsePosMask(const std::string& s);

// Position: decimal 0-255, digits only. -1 if invalid.
int parsePosition(const std::string& s);

// Phase lengths for a position command: each at most kPosMaxPhaseMs, and not
// both zero (0 skips that phase).
bool positionTimingOk(uint32_t phase1Ms, uint32_t phase2Ms);

// A remote ID we're willing to generate: anything but 00 00 and FF FF.
bool isUsableRemoteId(uint8_t id0, uint8_t id1);

// Picks a remote ID from 32 random bits: the low 16 bits (low byte first), or
// the high 16 if the low ones aren't usable. Returns false if neither half is;
// the caller draws again.
bool remoteIdFromRandom(uint32_t r, uint8_t out[2]);

// Transmit power names, numbered like RF24's rf24_pa_dbm_e (0 = MIN .. 3 = MAX).
// "min" | "low" | "high" | "max" (any case) -> 0..3; -1 if invalid.
int parsePaLevel(const std::string& s);
// 0..3 -> "min", "low", "high", "max"; "?" otherwise.
const char* paLevelName(int level);
// Output power of a genuine nRF24L01+ at that level: -18, -12, -6, 0 dBm.
// Clones (e.g. Si24R1) map the register bits differently. 99 if invalid.
int paLevelGenuineDbm(int level);

// "1".."4" -> 0x01, 0x02, 0x04, 0x08; "all" -> 0xFF (any case). -1 if invalid.
int parseGroup(const std::string& s);

// "1".."4", "All", "none", or "mask 0x03" for anything else.
std::string groupName(uint8_t group);
// "UP held", "DOWN held", "UP release", "DOWN release", "wake held",
// "wake release", or "?".
const char* actionName(uint8_t action);
// e.g. "ID 03C1  mode 20  group 1  UP held". Setup frames read "pair add held",
// "pair remove release", etc. Position frames read
// "ID 1234  mode 50  mask 08  pos 128". Empty unless len == kPayloadSize.
std::string describePayload(const uint8_t* p, size_t len);

}  // namespace rf
}  // namespace hd
