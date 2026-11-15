# SPDX-License-Identifier: GPL-2.0-or-later
"""Check the firmware's handheld pin map (include/pins.h) against the KiCad design.

Exports the schematic netlist with kicad-cli, reads which net each ESP32 module pin (U201) is on,
converts module pins to GPIO numbers, and compares that with every PIN_ constant in the
HD_BOARD_HANDHELD block of include/pins.h. Also reports ESP32 signal nets that pins.h doesn't name.

  python -I hardware/tools/check_pins.py            (from the repo root)

Exits 1 on any mismatch, so it can gate a commit. Needs KiCad 10's kicad-cli (KICAD_CLI to override).
"""
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from sexpr import parse  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
SCH = ROOT / "hardware" / "hd_remote" / "hd_remote.kicad_sch"
PINS_H = ROOT / "include" / "pins.h"
KICAD_CLI = os.environ.get(
    "KICAD_CLI", r"C:\Users\johnp\AppData\Local\Programs\KiCad\10.0\bin\kicad-cli.exe")
MODULE = "U201"

# ESP32-WROOM-32E module pin -> GPIO (Espressif datasheet, pin layout). Power, EN, NC and the
# exposed pad are omitted.
MODULE_GPIO = {
    4: 36, 5: 39, 6: 34, 7: 35, 8: 32, 9: 33, 10: 25, 11: 26, 12: 27, 13: 14, 14: 12,
    16: 13, 23: 15, 24: 2, 25: 0, 26: 4, 27: 16, 28: 17, 29: 5, 30: 18, 31: 19, 33: 21,
    34: 3, 35: 1, 36: 22, 37: 23,
}

# pins.h constant -> the net it must be on.
EXPECTED = {
    "PIN_IR_TX": "IR_TX",
    "PIN_BTN_UP": "BTN_UP", "PIN_BTN_DN": "BTN_DOWN", "PIN_BTN_ALL": "BTN_ALL",
    "PIN_BTN_G1": "BTN_G1", "PIN_BTN_G2": "BTN_G2", "PIN_BTN_G3": "BTN_G3", "PIN_BTN_G4": "BTN_G4",
    "PIN_BTN_SETUP": "BTN_SETUP",
    "PIN_AB_SW": "AB_SW_PIN",
    "PIN_VBAT_SENSE": "VBAT_SENSE",
    "PIN_LED_RED": "LED_RED", "PIN_LED_GREEN": "LED_GREEN",
    "PIN_NRF_CSN": "NRF_CSN", "PIN_NRF_CE": "NRF_CE", "PIN_NRF_IRQ": "NRF_IRQ_OPT",
}
# Nets on fixed pins the firmware doesn't name in pins.h (SPI defaults, UART0, boot).
FIXED = {"NRF_SCK": 18, "NRF_MISO": 19, "NRF_MOSI": 23, "U0TXD": 1, "U0RXD": 3, "IO0": 0}
NOT_SIGNALS = {"GND", "+3V3", "EN"}


def short(net):
    return net.split("/")[-1]


def module_nets():
    """GPIO -> net name, from the schematic netlist."""
    with tempfile.TemporaryDirectory() as d:
        out = Path(d) / "net.net"
        subprocess.run([KICAD_CLI, "sch", "export", "netlist", "--format", "kicadsexpr",
                        "-o", str(out), str(SCH)], check=True, capture_output=True)
        tree = parse(out.read_text(encoding="utf-8"))
    root = tree[0] if isinstance(tree[0], list) else tree
    nets = next(x for x in root if isinstance(x, list) and x and x[0] == "nets")
    gpio_net = {}
    for net in nets[1:]:
        name = next(x[1] for x in net if isinstance(x, list) and x[0] == "name")
        for node in (x for x in net if isinstance(x, list) and x[0] == "node"):
            f = {x[0]: x[1] for x in node[1:] if isinstance(x, list) and len(x) > 1}
            if f.get("ref") == MODULE and f.get("pin", "").isdigit():
                pin = int(f["pin"])
                if pin in MODULE_GPIO:
                    gpio_net[MODULE_GPIO[pin]] = short(name)
    return gpio_net


def handheld_pins():
    """PIN_ name -> GPIO from the HD_BOARD_HANDHELD block of pins.h (PIN_NONE skipped)."""
    text = PINS_H.read_text(encoding="utf-8")
    m = re.search(r"#if defined\(HD_BOARD_HANDHELD\)(.*?)#else", text, re.S)
    if not m:
        sys.exit("pins.h: no HD_BOARD_HANDHELD block found")
    pins = {}
    for name, value in re.findall(r"constexpr\s+uint8_t\s+(PIN_\w+)\s*=\s*(\w+)\s*;", m.group(1)):
        if value.isdigit():
            pins[name] = int(value)
    return pins


def main():
    gpio_net = module_nets()
    pins = handheld_pins()
    fails, lines = [], []

    for name, net in EXPECTED.items():
        if name not in pins:
            fails.append(f"{name}: missing from the handheld block of pins.h (net {net})")
            continue
        got = gpio_net.get(pins[name], "(not connected)")
        line = f"{name:15s} GPIO {pins[name]:2d}  net {got}"
        if got != net:
            fails.append(f"{name} = GPIO {pins[name]} is on net {got}, expected {net}")
            line += f"   <-- expected {net}"
        lines.append(line)
    for name in sorted(set(pins) - set(EXPECTED)):
        fails.append(f"{name} = GPIO {pins[name]}: not in this script's EXPECTED table (add it)")

    for net, gpio in FIXED.items():
        got = gpio_net.get(gpio, "(not connected)")
        if got != net:
            fails.append(f"fixed pin GPIO {gpio} is on net {got}, expected {net}")

    named = set(EXPECTED.values()) | set(FIXED)
    for gpio, net in sorted(gpio_net.items()):
        if net in NOT_SIGNALS or net.startswith("unconnected") or net in named:
            continue
        fails.append(f"GPIO {gpio} is on net {net}, which pins.h doesn't name")

    print("\n".join(lines))
    if fails:
        print("\nFAIL:\n  " + "\n  ".join(fails))
        sys.exit(1)
    print(f"\nok: {len(EXPECTED)} pins.h constants and {len(FIXED)} fixed pins match the schematic")


if __name__ == "__main__":
    main()
