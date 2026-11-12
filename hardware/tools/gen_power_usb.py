# SPDX-License-Identifier: GPL-2.0-or-later
"""Generates hardware/hd_remote/power_usb.kicad_sch (first draft) from
docs/HARDWARE.md: USB-C, ESD, CP2102N with auto-reset, LiPo charger, 3.3 V LDO,
battery sense. Run once; after editing the sheet in KiCad, don't rerun.

    python -I hardware/tools/gen_power_usb.py
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import schgen  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
PROJ_DIR = os.path.join(HERE, "..", "hd_remote")
ROOT = os.path.join(PROJ_DIR, "hd_remote.kicad_sch")
OUT = os.path.join(PROJ_DIR, "power_usb.kicad_sch")
PROJECT = "hd_remote"
SHEET_UUID = "2b0f6c1e-5d3a-4f7e-8a21-6c9d0e3f4a10"

R = "Resistor_SMD:R_0603_1608Metric"
C = "Capacitor_SMD:C_0603_1608Metric"
GLOBAL = ("EN", "IO0", "U0TXD", "U0RXD", "VBAT_SENSE")


def res(s, value, gx, gy, top, bottom, note=""):
    f = {"Footprint": R}
    if note:
        f["Description"] = note
    return s.part("Device:R_Small", "R", value, gx, gy, {"1": top, "2": bottom}, f, GLOBAL)


def cap(s, value, gx, gy, top, bottom):
    return s.part("Device:C_Small", "C", value, gx, gy, {"1": top, "2": bottom}, {"Footprint": C}, GLOBAL)


def build():
    s = schgen.Sheet(PROJECT, "power_usb.kicad_sch", "Power and USB", schgen.root_uuid(ROOT), SHEET_UUID,
                     ref_base=100)

    # --- USB-C receptacle (sink), CC pull-downs, ESD ---
    s.text("USB-C (USB 2.0, sink): 5.1k on CC1/CC2, USBLC6 on D+/D-", 8, 12)
    s.part("Connector:USB_C_Receptacle_USB2.0_16P", "J", "USB_C_Receptacle", 16, 32, {
        "A1": "GND", "A12": "GND", "B1": "GND", "B12": "GND", "SH": "GND",
        "A4": "VBUS", "A9": "VBUS", "B4": "VBUS", "B9": "VBUS",
        "A5": "CC1", "B5": "CC2",
        "A6": "USB_DP", "B6": "USB_DP", "A7": "USB_DN", "B7": "USB_DN",
        "A8": None, "B8": None,
    }, {"Footprint": "Connector_USB:USB_C_Receptacle_HRO_TYPE-C-31-M-12"}, GLOBAL)
    res(s, "5.1k", 30, 30, "CC1", "GND")
    res(s, "5.1k", 34, 30, "CC2", "GND")
    s.part("Power_Protection:USBLC6-2SC6", "U", "USBLC6-2SC6", 46, 32, {
        "1": "USB_DP", "6": "USB_DP", "3": "USB_DN", "4": "USB_DN", "5": "VBUS", "2": "GND",
    }, None, GLOBAL)

    # --- CP2102N USB-UART, bus-powered (VBUS only, draws nothing from the battery) ---
    s.text("CP2102N: powered from VBUS only. VBUS sense divider as on the CP2102N-EK.\n"
           "1k in series on TXD/RXD limits back-powering when USB is unplugged (verify sleep current).",
           60, 4)
    s.part("Interface_USB:CP2102N-Axx-xQFN24", "U", "CP2102N-A02-GQFN24", 80, 36, {
        "3": "USB_DP", "4": "USB_DN", "8": "CP_VBUS", "9": "CP_RST",
        "7": "VBUS", "6": "VDD_USB", "5": "VDD_USB", "2": "GND", "25": "GND",
        "21": "CP_TXD", "20": "CP_RXD", "23": "DTR", "19": "RTS",
        "1": None, "10": None, "11": None, "12": None, "13": None, "14": None,
        "15": None, "16": None, "17": None, "18": None, "22": None, "24": None,
    }, None, GLOBAL)
    cap(s, "4.7u", 62, 12, "VBUS", "GND")        # VREGIN bulk
    cap(s, "100n", 66, 12, "VBUS", "GND")        # VREGIN
    cap(s, "1u", 70, 12, "VDD_USB", "GND")       # VDD / VIO
    cap(s, "100n", 74, 12, "VDD_USB", "GND")
    res(s, "1k", 78, 12, "VDD_USB", "CP_RST", "RST pull-up to VIO (verify value)")
    res(s, "22.1k", 84, 12, "VBUS", "CP_VBUS", "VBUS sense divider, top")
    res(s, "47.5k", 88, 12, "CP_VBUS", "GND", "VBUS sense divider, bottom")
    res(s, "1k", 104, 28, "CP_TXD", "U0RXD", "Series: CP2102N TXD -> ESP32 RX0 (GPIO 3)")
    res(s, "1k", 108, 28, "U0TXD", "CP_RXD", "Series: ESP32 TX0 (GPIO 1) -> CP2102N RXD")

    # --- Auto-reset (DevKitC circuit): DTR/RTS -> EN / IO0 ---
    s.text("Auto-reset, as on the ESP32-DevKitC", 116, 20)
    res(s, "10k", 118, 30, "DTR", "Q1_B")
    s.part("Transistor_BJT:MMBT3904", "Q", "MMBT3904", 124, 30,
           {"1": "Q1_B", "2": "RTS", "3": "EN"}, None, GLOBAL)
    res(s, "10k", 118, 42, "RTS", "Q2_B")
    s.part("Transistor_BJT:MMBT3904", "Q", "MMBT3904", 124, 42,
           {"1": "Q2_B", "2": "DTR", "3": "IO0"}, None, GLOBAL)

    # --- LiPo charger ---
    s.text("Charger: MCP73831, ~250 mA (R_PROG 4.02k). Charge LED from VBUS, lit while charging.\n"
           "Load runs straight from VBAT (no power path).", 4, 66)
    s.part("Battery_Management:MCP73831-2-OT", "U", "MCP73831-2-OT", 20, 78,
           {"4": "VBUS", "2": "GND", "5": "PROG", "1": "STAT", "3": "VBAT"}, None, GLOBAL)
    cap(s, "4.7u", 8, 78, "VBUS", "GND")
    cap(s, "4.7u", 34, 78, "VBAT", "GND")
    res(s, "4.02k", 12, 90, "PROG", "GND", "I_charge = 1000 V / R_PROG = ~250 mA")
    s.text("TP: V(PROG) = 1 V at full\ncharge current (~250 mA)", 18, 96)
    s.part("Connector:TestPoint", "TP", "PROG", 20, 92, {"1": "PROG"},
           {"Footprint": "TestPoint:TestPoint_Pad_D1.5mm"}, GLOBAL)
    s.part("Device:LED_Small", "D", "CHG", 42, 78, {"1": "STAT", "2": "LED_CHG"},
           {"Footprint": "LED_SMD:LED_0603_1608Metric", "Description": "Charge LED (red or orange)"},
           GLOBAL)
    res(s, "1k", 48, 78, "VBUS", "LED_CHG")

    # --- Battery connector ---
    s.text("LiPo, 1 cell, protected. CHECK the cell's polarity against J2 before plugging in.", 54, 70)
    s.part("Connector_Generic:Conn_01x02", "J", "Battery", 62, 78, {"1": "VBAT_BATT", "2": "GND"},
           {"Footprint": "Connector_JST:JST_PH_S2B-PH-K_1x02_P2.00mm_Horizontal"}, GLOBAL)
    s.text("Battery current link: cut the bridge and put a uA meter\n"
           "across the pads to measure total battery current\n"
           "(sleep target ~11-16 uA). Re-bridge with solder after.", 50, 96)
    s.part("Jumper:SolderJumper_2_Bridged", "JP", "I_BAT", 58, 92, {"1": "VBAT_BATT", "2": "VBAT"},
           {"Footprint": "Jumper:SolderJumper-2_P1.3mm_Bridged_RoundedPad1.0x1.5mm"}, GLOBAL)

    # --- 3.3 V LDO ---
    s.text("LDO: RT9080-33GJ5 (2 uA Iq). Symbol is the AP2112K's: same SOT-23-5 pinout\n"
           "(1 VIN, 2 GND, 3 EN, 4 NC, 5 VOUT). Verify against the RT9080 datasheet.", 70, 66)
    s.part("Regulator_Linear:AP2112K-3.3", "U", "RT9080-33GJ5", 80, 78,
           {"1": "VBAT", "3": "VBAT", "2": "GND", "4": None, "5": "+3V3"}, None, GLOBAL)
    cap(s, "1u", 72, 88, "VBAT", "GND")
    cap(s, "1u", 88, 88, "+3V3", "GND")

    # --- Battery sense ---
    s.text("Battery sense to GPIO 39 (ADC1_CH3): VBAT / 2, ~2 uA", 96, 94)
    res(s, "1M", 100, 78, "VBAT", "VBAT_SENSE")
    res(s, "1M", 104, 78, "VBAT_SENSE", "GND")
    cap(s, "100n", 108, 78, "VBAT_SENSE", "GND")

    # --- Power flags: the connectors' power pins are passive, and the symbol
    # marks the CP2102N's VDD (its regulator output) as an input. VBAT needs
    # none: the charger's VBAT pin is a power output.
    s.flag("VBUS", 116, 90)
    s.flag("GND", 124, 90)
    s.flag("VDD_USB", 134, 90)

    text = s.render({
        "title": "HD Platinum remote replacement: Power and USB",
        "date": "2026-11-12",
        "rev": "0.1",
        "comments": ["Draft, generated by hardware/tools/gen_power_usb.py", "Spec: docs/HARDWARE.md"],
    })
    with open(OUT, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    added = schgen.add_sheet_to_root(ROOT, PROJECT, SHEET_UUID, "Power and USB", "power_usb.kicad_sch",
                                     2, 10, 10)
    print("wrote", OUT, "| root sheet block", "added" if added else "already present")
    nets = {}
    for ref, pin, net in s.netlist:
        nets.setdefault(net, []).append("%s.%s" % (ref, pin))
    for net in sorted(nets, key=lambda n: (n is None, str(n))):
        print("  %-12s %s" % (net, " ".join(nets[net])))


if __name__ == "__main__":
    build()
