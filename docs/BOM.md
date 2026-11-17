# Parts list (handheld board rev 1)

Everything needed to build one handheld: the board's parts (generated from the schematic), the parts that aren't on the board, and notes. The *reasons* for each choice are in [HARDWARE.md](HARDWARE.md); this file says what to buy and what's still open.

**Regenerating the board table:** after any schematic change, from the repo root:

```
python -I hardware/tools/lcsc_fetch.py --all     # only if an LCSC number was added or changed
python -I hardware/tools/make_bom.py
```

`make_bom.py` rewrites only the table between the GENERATED markers below. Manufacturer, MPN, datasheet and description come from the schematic's fields where set, otherwise from LCSC's page for the part's LCSC number (cached in `hardware/bom/lcsc_cache.json`). To change a board part, change the schematic, not this table.

**Fit column:** *JLC* = SMD, assembled by JLCPCB. *JLC (THT) or hand* = through-hole with an LCSC number: JLC's paid THT assembly, or solder it yourself. *hand* = not stocked by LCSC; buy it separately and solder it by hand.

## Board parts

<!-- BEGIN GENERATED: hardware/tools/make_bom.py -->

43 lines, 93 fitted parts: 77 SMD parts JLC assembles, 16 through-hole or hand-fitted; plus 2 footprints left empty (DNP, population options). Generated 2026-11-16 from `hardware/hd_remote/hd_remote.kicad_sch`; don't edit this table by hand. **No part chosen yet:** TP604–TP606.

| Ref | Qty | Value | Manufacturer | MPN | LCSC | Fit | Datasheet | Description |
|---|---|---|---|---|---|---|---|---|
| C101, C103, C105, C106 | 4 | 4.7u | Samsung Electro-Mechanics | CL10A475KO8NNNC | [C19666](https://www.lcsc.com/product-detail/C19666.html) | JLC | [PDF](https://datasheet.lcsc.com/datasheet/pdf/02336ea48ea44ca18c72517dd3cb7b47.pdf?productCode=C19666) | Unpolarized capacitor, small symbol |
| C102, C104, C109, C202, C302, C402, C501–C508 | 14 | 100n | YAGEO | CC0603KRX7R9BB104 | [C14663](https://www.lcsc.com/product-detail/C14663.html) | JLC | [PDF](https://datasheet.lcsc.com/datasheet/pdf/23ccee80ee542e7cf156a772bb589942.pdf?productCode=C14663) | Unpolarized capacitor, small symbol |
| C107, C108, C203 | 3 | 1u | Samsung Electro-Mechanics | CL10A105KB8NNNC | [C15849](https://www.lcsc.com/product-detail/C15849.html) | JLC | [PDF](https://datasheet.lcsc.com/datasheet/pdf/02336ea48ea44ca18c72517dd3cb7b47.pdf?productCode=C15849) | Unpolarized capacitor, small symbol |
| C201 | 1 | 22u | Samsung Electro-Mechanics | CL21A226MAQNNNE | [C45783](https://www.lcsc.com/product-detail/C45783.html) | JLC | [PDF](https://datasheet.lcsc.com/datasheet/pdf/02336ea48ea44ca18c72517dd3cb7b47.pdf?productCode=C45783) | Unpolarized capacitor, small symbol |
| C301 | 1 | 100u | Samsung Electro-Mechanics | CL31A107MQHNNNE | [C15008](https://www.lcsc.com/product-detail/C15008.html) | JLC | [PDF](https://datasheet.lcsc.com/datasheet/pdf/02336ea48ea44ca18c72517dd3cb7b47.pdf?productCode=C15008) | Unpolarized capacitor, small symbol |
| C401 | 1 | 10u | Samsung Electro-Mechanics | CL21A106KAYNNNE | [C15850](https://www.lcsc.com/product-detail/C15850.html) | JLC | [PDF](https://datasheet.lcsc.com/datasheet/pdf/02336ea48ea44ca18c72517dd3cb7b47.pdf?productCode=C15850) | Unpolarized capacitor, small symbol |
| C403 | 1 | 100u | Samsung Electro-Mechanics | CL31A107MQHNNNE | [C15008](https://www.lcsc.com/product-detail/C15008.html) | **DNP** (not fitted) | [PDF](https://datasheet.lcsc.com/datasheet/pdf/02336ea48ea44ca18c72517dd3cb7b47.pdf?productCode=C15008) | Bulk for a PA/LNA module only |
| D101 | 1 | CHG | Kingbright | APA1606SURCK | [C3034753](https://www.lcsc.com/product-detail/C3034753.html) | JLC | [PDF](https://datasheet.lcsc.com/datasheet/pdf/122e2364290b5f1df75f9b7478d32ba1.pdf?productCode=C3034753) | Charge LED, red, side-view (Kingbright APA1606SURCK); at the back edge, shines through a hole in the end wall |
| D102 | 1 | STPS1H100AY | ST | STPS1H100AY | [C181833](https://www.lcsc.com/product-detail/C181833.html) | JLC | [PDF](https://datasheet.lcsc.com/datasheet/pdf/e36c8d0b90ddefa44b5149020fd5d96a.pdf?productCode=C181833) | Schottky diode, 100 V 1 A, low leakage (power path) |
| D301, D302 | 2 | 1N4148W | ST(Semtech) | 1N4148W | [C81598](https://www.lcsc.com/product-detail/C81598.html) | JLC | [PDF](https://datasheet.lcsc.com/datasheet/pdf/8abd7fc00ebe41ffb03ad1383c10753b.pdf?productCode=C81598) | 75V 0.15A Fast Switching Diode, SOD-123 |
| D303, D304 | 2 | TSAL6200 | Vishay | TSAL6200 | — | hand | [PDF](https://www.vishay.com/docs/81010/tsal6200.pdf) | 940 nm IR LED, +-17 deg (or a wider-angle part in one position) |
| D501 | 1 | Red/Green CC | Kingbright | WP59EGW | — | hand | [PDF](https://www.kingbrightusa.com/images/catalog/SPEC/WP59EGW.pdf) | 5 mm bi-color LED, red/green, common cathode, white diffused (1 = red anode, 2 = cathode, 3 = green anode). Seated flush on the board, under a Bivar PLP2-250 light pipe |
| FB401 | 1 | 0R (or bead) | UNI-ROYAL | 0603WAF0000T5E | [C21189](https://www.lcsc.com/product-detail/C21189.html) | JLC | [PDF](https://datasheet.lcsc.com/datasheet/pdf/f42da6c80a0747bae77c2f98f4e46d1d.pdf?productCode=C21189) | 0 ohm link by default; ferrite bead option for the nRF24 supply |
| J101 | 1 | USB_C_Receptacle | Korean Hroparts Elec | TYPE-C-31-M-12 | [C165948](https://www.lcsc.com/product-detail/C165948.html) | JLC | [PDF](https://datasheet.lcsc.com/datasheet/pdf/9e56b777c022540fcce7c7f67825f55e.pdf?productCode=C165948) | USB 2.0-only 16P Type-C Receptacle connector |
| J102 | 1 | Battery | JST | S2B-PH-K-S(LF)(SN) | [C173752](https://www.lcsc.com/product-detail/C173752.html) | JLC (THT) or hand | [PDF](https://datasheet.lcsc.com/datasheet/pdf/6c6fce4367275d84ec6a99bf09f1250f.pdf?productCode=C173752) | Generic connector, single row, 01x02, script generated (kicad-library-utils/schlib/autogen/connector/) |
| J401 | 1 | nRF24_socket | Samtec | SLW-104-01-G-D | — | hand | [PDF](https://suddendocs.samtec.com/catalog_english/slw.pdf) | nRF24 module socket: low-profile 2x4 female, 2.54 mm, 4.57 mm tall (decided 2026-11-16, John). Trim the module's header pins to ~2.6 mm (insertion depth 2.16-2.92 mm) |
| JP101 | 1 | 0R | UNI-ROYAL | 0805W8F0000T5E | [C17477](https://www.lcsc.com/product-detail/C17477.html) | JLC | [PDF](https://datasheet.lcsc.com/datasheet/pdf/0a975aaa49b7c97f38a963127be4a823.pdf?productCode=C17477) | 0 ohm link in the battery path (I_BAT): lift it to measure battery current, then refit. Drawn with the solder-jumper symbol |
| Q101, Q102 | 2 | MMBT3904 | JSCJ | MMBT3904(RANGE:100-300) | [C20526](https://www.lcsc.com/product-detail/C20526.html) | JLC | [PDF](https://datasheet.lcsc.com/datasheet/pdf/9335632bcfa83df8351353eab379720e.pdf?productCode=C20526) | 0.2A Ic, 40V Vce, Small Signal NPN Transistor, SOT-23 |
| Q103, Q104 | 2 | AO3401A | AOS | AO3401A | [C15127](https://www.lcsc.com/product-detail/C15127.html) | JLC | [PDF](https://datasheet.lcsc.com/datasheet/pdf/fee353dd1e9e0bc90b295f14f381aa4c.pdf?productCode=C15127) | Reverse-polarity protection; any SOT-23 P-FET with /Vgs(th)/ < ~1.5 V, Vgs max >= 8 V |
| Q301, Q302 | 2 | BC817-40 | Nexperia | BC817-40,215 | [C52801](https://www.lcsc.com/product-detail/C52801.html) | JLC | [PDF](https://datasheet.lcsc.com/datasheet/pdf/82b42ce666a535f4f2a2391d7fd3bc11.pdf?productCode=C52801) | 0.8A Ic, 45V Vce, NPN Transistor, SOT-23 |
| R101, R102 | 2 | 5.1k | UNI-ROYAL | 0603WAF5101T5E | [C23186](https://www.lcsc.com/product-detail/C23186.html) | JLC | [PDF](https://datasheet.lcsc.com/datasheet/pdf/0a975aaa49b7c97f38a963127be4a823.pdf?productCode=C23186) | Resistor, small symbol |
| R103, R106, R111 | 3 | 1k | UNI-ROYAL | 0603WAF1001T5E | [C21190](https://www.lcsc.com/product-detail/C21190.html) | JLC | [PDF](https://datasheet.lcsc.com/datasheet/pdf/0a975aaa49b7c97f38a963127be4a823.pdf?productCode=C21190) | RST pull-up to VIO (verify value) |
| R104 | 1 | 22k | UNI-ROYAL | 0603WAF2202T5E | [C31850](https://www.lcsc.com/product-detail/C31850.html) | JLC | [PDF](https://datasheet.lcsc.com/datasheet/pdf/0a975aaa49b7c97f38a963127be4a823.pdf?productCode=C31850) | VBUS sense divider, top |
| R105 | 1 | 47k | UNI-ROYAL | 0603WAF4702T5E | [C25819](https://www.lcsc.com/product-detail/C25819.html) | JLC | [PDF](https://datasheet.lcsc.com/datasheet/pdf/0a975aaa49b7c97f38a963127be4a823.pdf?productCode=C25819) | VBUS sense divider, bottom |
| R107–R109, R201, R202 | 5 | 10k | UNI-ROYAL | 0603WAF1002T5E | [C25804](https://www.lcsc.com/product-detail/C25804.html) | JLC | [PDF](https://datasheet.lcsc.com/datasheet/pdf/0a975aaa49b7c97f38a963127be4a823.pdf?productCode=C25804) | Series: ESP32 TX0 (GPIO 1) -> CP2102N RXD |
| R110 | 1 | 3.9k | UNI-ROYAL | 0603WAF3901T5E | [C23018](https://www.lcsc.com/product-detail/C23018.html) | JLC | [PDF](https://datasheet.lcsc.com/datasheet/pdf/0a975aaa49b7c97f38a963127be4a823.pdf?productCode=C23018) | I_charge = 1000 V / R_PROG = ~250 mA |
| R112, R113 | 2 | 1M | UNI-ROYAL | 0603WAF1004T5E | [C22935](https://www.lcsc.com/product-detail/C22935.html) | JLC | [PDF](https://datasheet.lcsc.com/datasheet/pdf/0a975aaa49b7c97f38a963127be4a823.pdf?productCode=C22935) | Resistor, small symbol |
| R114, R302, R402, R403, R501–R508 | 12 | 100k | UNI-ROYAL | 0603WAF1003T5E | [C25803](https://www.lcsc.com/product-detail/C25803.html) | JLC | [PDF](https://datasheet.lcsc.com/datasheet/pdf/0a975aaa49b7c97f38a963127be4a823.pdf?productCode=C25803) | Resistor, small symbol |
| R301 | 1 | 470 | UNI-ROYAL | 0603WAF4700T5E | [C23179](https://www.lcsc.com/product-detail/C23179.html) | JLC | [PDF](https://datasheet.lcsc.com/datasheet/pdf/0a975aaa49b7c97f38a963127be4a823.pdf?productCode=C23179) | Resistor, small symbol |
| R303, R304 | 2 | 3.3 | FOJAN | FRC0805F3R30TS | [C2907250](https://www.lcsc.com/product-detail/C2907250.html) | JLC | [PDF](https://datasheet.lcsc.com/datasheet/pdf/106fb934f95f063b695dad1b31b1eab8.pdf?productCode=C2907250) | Sets I_LED ~150 mA (value from the breadboard); ~0.1 W peak, so 0805 |
| R401 | 1 | 0 | UNI-ROYAL | 0603WAF0000T5E | [C21189](https://www.lcsc.com/product-detail/C21189.html) | **DNP** (not fitted) | [PDF](https://datasheet.lcsc.com/datasheet/pdf/f42da6c80a0747bae77c2f98f4e46d1d.pdf?productCode=C21189) | Resistor, small symbol |
| R509 | 1 | 330 | UNI-ROYAL | 0603WAF3300T5E | [C23138](https://www.lcsc.com/product-detail/C23138.html) | JLC | [PDF](https://datasheet.lcsc.com/datasheet/pdf/0a975aaa49b7c97f38a963127be4a823.pdf?productCode=C23138) | Resistor, small symbol |
| R510 | 1 | 270 | UNI-ROYAL | 0603WAF2700T5E | [C22966](https://www.lcsc.com/product-detail/C22966.html) | JLC | [PDF](https://datasheet.lcsc.com/datasheet/pdf/0a975aaa49b7c97f38a963127be4a823.pdf?productCode=C22966) | Resistor, small symbol |
| SW201, SW202 | 2 | RESET, BOOT | C&K | KMR221NGLFS | [C269272](https://www.lcsc.com/product-detail/C269272.html) | JLC | [PDF](https://datasheet.lcsc.com/datasheet/pdf/1daa78c4b09299d7c6fc0a1267a41ee9.pdf?productCode=C269272) | Push button switch, generic, two pins |
| SW501–SW503, SW505–SW508 | 7 | UP, DOWN, ALL, 1, 2, 3, 4 | Same Sky | TS02-66-130-BK-160-SCR-D | — | hand | — | Push button switch, generic, two pins |
| SW504 | 1 | SETUP | Same Sky | TS02-66-50-BK-160-SCR-D | — | hand | — | Push button switch, generic, two pins |
| SW509 | 1 | A/B | C&K | PCM12SMTR | [C221841](https://www.lcsc.com/product-detail/C221841.html) | JLC | [PDF](https://datasheet.lcsc.com/datasheet/pdf/677f227074f0d201058521a10a93c772.pdf?productCode=C221841) | Switch, single pole double throw |
| TP604–TP606 | 3 | GND | — | **to choose** | — | hand | — | test point |
| U101 | 1 | USBLC6-2SC6 | ST | USBLC6-2SC6 | [C7519](https://www.lcsc.com/product-detail/C7519.html) | JLC | [PDF](https://datasheet.lcsc.com/datasheet/pdf/0d3a2ab954b34651a0695e7ccf534db0.pdf?productCode=C7519) | Very low capacitance ESD protection diode, 2 data-line, SOT-23-6 |
| U102 | 1 | CP2102N-A02-GQFN24 | SILICON LABS | CP2102N-A02-GQFN24R | [C969151](https://www.lcsc.com/product-detail/C969151.html) | JLC | [PDF](https://datasheet.lcsc.com/datasheet/pdf/11ab89ca3026e9cb7cc85ffe9557abff.pdf?productCode=C969151) | USB to UART master bridge, QFN-24 |
| U103 | 1 | MCP73831-2-OT | MICROCHIP | MCP73831T-2ACI/OT | [C424093](https://www.lcsc.com/product-detail/C424093.html) | JLC | [PDF](https://datasheet.lcsc.com/datasheet/pdf/c7ec00710e742b7fb9393e620ae1332b.pdf?productCode=C424093) | Single cell, Li-Ion/Li-Po charge management controller, 4.20V, Tri-State Status Output, in SOT23-5 package |
| U104 | 1 | RT9080-33GJ5 | RICHTEK | RT9080-33GJ5 | [C841192](https://www.lcsc.com/product-detail/C841192.html) | JLC | [PDF](https://datasheet.lcsc.com/datasheet/pdf/2701e0c3a2b84b9965f7a61f2c14e970.pdf?productCode=C841192) | 600 mA LDO, 3.3 V fixed, with enable, ~2 uA quiescent, SOT-23-5 (drawn with the AP2112K symbol; same pinout) |
| U201 | 1 | ESP32-WROOM-32E | ESPRESSIF | ESP32-WROOM-32E-N4 | [C701341](https://www.lcsc.com/product-detail/C701341.html) | JLC | [PDF](https://datasheet.lcsc.com/datasheet/pdf/d5deb62e2fbb5c62d76522c63d752c06.pdf?productCode=C701341) | RF Module, ESP32-D0WD-V3 SoC, without PSRAM, Wi-Fi 802.11b/g/n, Bluetooth, BLE, 32-bit, 2.7-3.6V, onboard antenna, SMD |

<!-- END GENERATED -->

Not listed (no part to buy): test pads TP101–TP303, TP601–TP603 and TP607–TP613, mounting holes H601–H604.

**DNP rows** are footprints deliberately left empty, so their part isn't bought or placed: C403 (extra nRF24 bulk for a PA/LNA module) and R401 (the nRF24 IRQ link to GPIO 13). They're listed with a part number so they can be fitted later. **J102** (the battery JST connector) has an LCSC number but is hand-fitted: decided 2026-11-16 (John) not to use JLC's through-hole service. `hardware/tools/make_jlc.py` leaves it out of the JLC BOM and placement files by default. The key debounce caps C501–C508 were DNP too, until 2026-11-15; JLC now fits them (same 100 nF part as the board's other decoupling caps).

## Parts not on the board

| Item | Qty | Part | Source | Notes |
|---|---|---|---|---|
| Battery | 1 | 500 mAh 3.7 V LiPo with JST-PH plug, PKCELL LP-503035 ([datasheet](https://cdn-shop.adafruit.com/product-files/1578/Datasheet.pdf)) | [Adafruit #1578](https://www.adafruit.com/product/1578) | Built-in protection (S-8261AAJMD). See appendix: polarity, width |
| nRF24L01+ module | 1 | 2×4-header module with PCB antenna | hiBCTR 10-pack (on hand) | Plugs into J401, antenna toward the IR end. Possibly Si24R1 clones: screen with `test` first |
| Board screws | 4 | M2 self-tapping, ~6 mm | any | Into the case standoffs |
| Indicator light pipe | 1 | Bivar PLP2-250: clear polycarbonate, 3 mm round, 6.35 mm below its 3.3 × 0.8 mm head ([drawing](https://www.bivar.com/parts_content/Datasheets/PLP2-XXX.pdf)) | [DigiKey 586461](https://www.digikey.com/en/products/detail/bivar-inc/PLP2-250/586461) | Presses into a 2.92 mm hole in the key face (panel 1.2–2.4 mm; ours is 2.0). Ends ~0.95 mm above the LED dome. Glue it if the printed hole is loose |
| Case | 1 set | Bottom and top shells, 7 keycaps, A/B slider | 3D print (JLC3DP MJF PA12 used for the test piece) | From `hardware/enclosure/` |

## Appendix: notes and open items

**Open: choose before ordering**

1. **D501, bicolor indicator LED: chosen 2026-11-15 (John), Kingbright WP59EGW** (DigiKey 1747631). Its datasheet (rev V.14A, 2025) confirms common cathode and the footprint's lead order: 1 = red anode (square pad), 2 = common cathode, 3 = green anode, 2.54 mm pitch. Red 2.0 V / green 2.2 V (GaP, 568 nm) typical, so R509/R510 stay as they are (~4–5 mA each). Seat it **flush on the board** (keep soldering to 2–3 s per lead: the joint is ~1.6 mm below the lens, the datasheet rates 2 mm) and diode-test the leads before fitting. The light reaches the face through a light pipe (below). Not the W59EGW-CA, which is common anode.
2. **J401, nRF24 socket:** **Decided 2026-11-16 (John): Samtec SLW-104-01-G-D** low-profile socket (2×4, 4.57 mm insulator, 20 µin gold; DigiKey 612-SLW-104-01-G-D-ND), with the header pins of each module used in a handheld **trimmed to ~2.6 mm** below the plastic spacer (they're 5.3–5.4 mm; the SLW takes 2.16–2.92 mm). Rejected: a standard 8.5 mm socket with a raised area on the top shell (looks awkward), and soldering the module directly (antenna ~8 mm above the cell; RF margin unknown). Details: [HARDWARE.md, nRF24 socket](HARDWARE.md#nrf24-socket). Pin 1 checked against a real module 2026-11-16.
3. **TP604–TP606, ground test loops:** Keystone 5000-series miniature loops; pick a colour (5001 is black). Optional.

**Check on arrival**

4. **Battery width:** Adafruit lists 29 × 36 × 4.75 mm; the cell datasheet's drawing says **30** × 35 × 5.0 mm (±0.1). Since 2026-11-15 the case model uses 30 mm (`BATT_W`), so the rib pocket is 31 mm: 0.5 mm play per side for a 30 mm cell, 1.0 mm for a 29 mm one, never a press fit. The board's battery keep-out was already 31 mm. Still measure the real cell, including the bulge at its protection board, and rerun `assembly.py` with the measured size.
5. **J401 orientation:** the nRF24's 2×4 header fits either way round, and reversed it puts 3.3 V on the wrong pins. The silkscreen shows the module's outline and "nRF24 ANT →": fit the module so its antenna end lies toward the IR LEDs, inside that outline.
6. **Battery polarity (checked on paper 2026-11-15):** J102 pin 1 = + (VBAT_BATT). Adafruit's own charger board design (`Adafruit USB C microlipo charger`, [GitHub](https://github.com/adafruit/Adafruit-MicroLipo-PCB)) puts + on the same side of the socket (left, looking into the opening with the board underneath), and Adafruit says its cells match its boards. The cell datasheet marks the plug "Reverse the plug, JST PH-2P", i.e. reversed from the cell maker's default, which is how it matches Adafruit. **Still meter the plug before the first connection** ([BRINGUP.md](BRINGUP.md), stage 7); Q103 blocks a reversed cell.

**Notes**

7. **U104 (RT9080-33GJ5)** is drawn with KiCad's AP2112K-3.3 symbol (same SOT-23-5 pinout: 1 VIN, 2 GND, 3 EN, 4 NC, 5 VOUT). Its datasheet field pointed at the AP2112 until 2026-11-15; it now points at the RT9080.
8. **SW509, A/B switch:** C&K PCM12SMTR (LCSC C221841), matching the footprint. The case's slider also fits an MSK12C02, but that needs a different footprint.
9. **Key switches SW501–SW508:** Same Sky TS02 from DigiKey (100 ordered 2026-11-13): 13 mm for the seven top keys, 5 mm for SETUP on the underside ([HARDWARE.md, Key switch sourcing](HARDWARE.md#key-switch-sourcing-checked-2026-11-13)).
10. **D101, charge LED:** Kingbright APA1606SURCK, a red side-view LED seen through the end-wall window.
11. **D301/D302, 1N4148W:** LCSC lists the maker as "ST(Semtech)", a Chinese manufacturer, not STMicroelectronics or the US Semtech; use the linked datasheet, not st.com.
12. **Datasheet links:** for parts with an LCSC number, the table always links LCSC's copy of the datasheet for *that exact part* (the schematic's own link is often a generic family PDF or another maker's version), so a pinout review checks what JLC actually fits. LCSC's links can change: refresh with `lcsc_fetch.py --all --refresh`.
13. **JP101, battery current link:** a 0 Ω 0805 since 2026-11-15 (was a bridged solder jumper). Lift it to measure battery current, then refit it; that survives repeated sleep-current measurements better than cutting and re-bridging a jumper ([BRINGUP.md](BRINGUP.md), stage 7).
