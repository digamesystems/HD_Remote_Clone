#pragma once
// Collapses a packet stream into runs of identical payloads, so the sniffer
// can count every packet without printing each one. The original remote sends
// thousands of packets per second while a key is held; a serial line per
// packet would stall the loop and overflow the radio's 3-packet FIFO.
//
// A run ends when a different payload arrives or the stream goes idle.

#include <cstdint>

namespace hd {

struct PacketRun {
  static constexpr uint8_t kMaxLen = 32;
  uint8_t payload[kMaxLen];
  uint8_t len;
  uint32_t count;
  uint32_t firstUs;   // timestamp of the first packet
  uint32_t lastUs;    // timestamp of the last packet
  uint32_t minGapUs;  // shortest gap between packets in the run (0 if count < 2)
  uint32_t maxGapUs;  // longest gap between packets in the run
};

class RunTracker {
 public:
  void reset() { active_ = false; }

  // Adds a packet received at tUs. If it ends the current run (different
  // payload), copies the finished run to *done and returns true.
  bool feed(const uint8_t* p, uint8_t len, uint32_t tUs, PacketRun* done);

  // Ends the current run if nothing has arrived for idleUs.
  bool flushIfIdle(uint32_t nowUs, uint32_t idleUs, PacketRun* done);

  // Ends the current run unconditionally (e.g. when sniffing stops).
  bool flush(PacketRun* done);

 private:
  void start(const uint8_t* p, uint8_t len, uint32_t tUs);
  PacketRun cur_{};
  bool active_ = false;
};

}  // namespace hd
