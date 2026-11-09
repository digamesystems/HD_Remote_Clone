#pragma once
#include <Arduino.h>

namespace rf {

void begin();

// Repeated SPI/register check on the attached module, per data rate. PASS
// needs clean channel/PA read-back and 2 Mbps (the Platinum remote's rate).
void selfTest(int moduleLabel);
void printDetails();

// Carrier-detect (RPD) sweep over channels 0..125. Prints one row per pass.
void startScan();
// Listen on the current config and print received payloads.
void startSniff();
// Loopback: radio B (pins.h) sends `count` beacon packets with the current
// config while the main radio receives, then prints sent/received.
// Returns false for a bad count.
bool startBeacon(int count);
void stop();
bool busy();  // a scan, sniff or loopback is running
void poll();  // call from loop()

// Config (applied when a scan/sniff starts). Defaults to the Platinum handheld.
void setChannel(uint8_t ch);
bool setRate(const String& s);  // "250k" | "1m" | "2m"
bool setAddressWidth(uint8_t w);  // 2..5 (2 = promiscuous trick)
bool setAddress(const String& hex);  // up to 5 bytes, hex, as seen on air (MSB first)
bool setCrc(uint8_t bytes);  // 0, 1, 2
bool setPayloadSize(uint8_t n);  // 1..32
void presetHd();  // Platinum handheld: ch 33, 2 Mbps, E7E7E7, CRC16, 5 bytes, runs output
void presetPromiscuous(uint8_t preamble);  // 0xAA or 0x55; turns the dup filter on
void setDupFilter(bool on);  // on: print payloads seen 2+ times; off: print runs
void printConfig();

}  // namespace rf
