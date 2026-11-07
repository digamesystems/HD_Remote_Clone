#pragma once
#include <Arduino.h>

namespace rf {

void begin();

// Runs a repeated SPI/register check on the attached module and prints PASS/FAIL.
void selfTest(int moduleLabel);
void printDetails();

// Carrier-detect (RPD) sweep over channels 0..125. Prints one row per pass.
void startScan();
// Listen on the current config and print received payloads.
void startSniff();
void stop();
void poll();  // call from loop()

// Config (applied when a scan/sniff starts)
void setChannel(uint8_t ch);
bool setRate(const String& s);  // "250k" | "1m" | "2m"
bool setAddressWidth(uint8_t w);  // 2..5 (2 = promiscuous trick)
bool setAddress(const String& hex);  // up to 5 bytes, hex, as seen on air (MSB first)
bool setCrc(uint8_t bytes);  // 0, 1, 2
bool setPayloadSize(uint8_t n);  // 1..32
void presetPromiscuous(uint8_t preamble);  // 0xAA or 0x55
void setDupFilter(bool on);  // print only payloads seen 2+ times
void printConfig();

}  // namespace rf
