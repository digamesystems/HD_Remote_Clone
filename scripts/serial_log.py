"""Send commands to the bring-up firmware and log its serial output to a file.

Usage (from the project root, with no serial monitor holding the port):
    python scripts/serial_log.py --cmd ir --seconds 60 --name ir_red_mode
Writes captures/<name>_<timestamp>.log and echoes output to the console.
Press Ctrl+C to stop early.
"""

import argparse
import datetime as dt
import pathlib
import sys
import time

import serial  # pyserial (bundled with PlatformIO's Python)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", default="COM11")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--cmd", action="append", default=[], help="command to send (repeatable)")
    ap.add_argument("--seconds", type=float, default=60.0)
    ap.add_argument("--name", default="session")
    args = ap.parse_args()

    out_dir = pathlib.Path(__file__).resolve().parent.parent / "captures"
    out_dir.mkdir(exist_ok=True)
    stamp = dt.datetime.now().strftime("%Y%m%d_%H%M%S")
    path = out_dir / f"{args.name}_{stamp}.log"

    # Opening the port with DTR/RTS deasserted avoids resetting the ESP32.
    ser = serial.Serial()
    ser.port, ser.baudrate, ser.timeout = args.port, args.baud, 0.2
    ser.dtr = False
    ser.rts = False
    ser.open()
    time.sleep(0.3)
    ser.reset_input_buffer()

    with path.open("w", encoding="utf-8") as f:
        def emit(text: str) -> None:
            f.write(text)
            f.flush()
            sys.stdout.write(text)
            sys.stdout.flush()

        emit(f"# {path.name}  port={args.port}  cmds={args.cmd}\n")
        for c in args.cmd:
            ser.write((c + "\n").encode())
            time.sleep(0.2)
        t_end = time.time() + args.seconds
        try:
            while time.time() < t_end:
                data = ser.read(4096)
                if data:
                    emit(data.decode("utf-8", errors="replace"))
        except KeyboardInterrupt:
            emit("\n# stopped by user\n")
        ser.write(b"\n")  # stop capture mode
        time.sleep(0.2)
        emit(ser.read(4096).decode("utf-8", errors="replace"))
    ser.close()
    print(f"\nSaved {path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
