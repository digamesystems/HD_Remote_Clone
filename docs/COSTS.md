# Landed costs

What the prototypes actually cost to get into hand, per board and per case. Updated as orders are placed or arrive. The figures come from the order and cart screens (no addresses or payment details are kept here).

**Conventions** (proposed; change them if another split suits better):
- **Landed** means merchandise plus shipping, customs duties and taxes.
- Order-level shipping, duties and taxes are shared across an order's lines in proportion to their merchandise price.
- Spares count as part of the batch's cost: if 10 sockets are bought for 5 boards, all 10 are charged to the batch.
- Per-board and per-case figures are the batch cost divided by the number built. Test gear and one-off development items are listed apart and are not spread over the units.

## Ledger

| Date | Vendor | What | Merchandise | Shipping | Duties and tax | Landed | Charged to |
|---|---|---|---|---|---|---|---|
| 2026-11-17 | JLCPCB | 5 assembled boards (PCB $17.69, PCBA $148.20) | $165.89 | $37.92 | $82.17 | $285.98 | Board batch (5) |
| 2026-11-17 | DigiKey | J401 sockets ×10, D501 ×10, light pipes ×15, J102 ×10 | $25.40 | $7.40 | $6.11 | $38.91 | Board batch (5) |
| 2026-11-17 | DigiKey | IR sink-test parts (diodes, PN2222A, resistors) | $3.76 | $1.09 | $0.91 | $5.76 | Development |
| 2026-11-17 | Adafruit | Battery #1578 ×5 | $39.75 | $19.67 | $3.98 | $63.40 | Board batch (5), battery |
| 2026-11-17 | Amazon | 2 Bridgold LED kits (10 TSAL6200 + 10 receivers) | not recorded | | | not recorded | Board batch (5) |
| 2026-11-13 | JLC3DP | Enclosure test piece (7 parts) | $10.79 | | | landed not recorded | Development (case) |

JLCPCB's duties and taxes are customs $58.07, disbursement fee $7.52 and sales tax $16.58. The DigiKey order's shipping ($8.49), tariff ($3.73) and sales tax ($3.29) total $44.67 with the $29.16 of merchandise, split between its two lines by merchandise price.

## Summary

**Board batch, 5 boards** (landed, without the Amazon LED kits that aren't recorded yet):

| | Batch | Per board |
|---|---|---|
| JLCPCB, PCB and assembly | $285.98 | $57.20 |
| DigiKey, hand-fitted parts (with spares) | $38.91 | $7.78 |
| **Electronics, without the LEDs** | **$324.89** | **$64.98** |
| Batteries | $63.40 | $12.68 |
| **Electronics plus battery, without the LEDs** | **$388.29** | **$77.66** |

Add the LED kits' landed cost when it's recorded (about $3–4 per board at roughly $18 for the batch).

**Case:** no case has been bought yet, so there is no per-case figure. The test piece ($10.79 merchandise) is development, not a case. Record each case print order as a row above, then divide by the cases it makes.

**Development, not spread over the units:** IR sink-test parts $5.76; the test piece.

**One-time costs inside the JLCPCB figure** (the same for 5 boards or 50): setup $25.75, stencil $8.27, engineering $4.00, feeder loading $52.70, together $90.72. A larger batch spreads them, and a second batch would repeat them.

## Updating this file

When an order is placed, add a row with its merchandise, shipping, duties and tax, and what it's charged to, then update the summary. When an order arrives with a different landed cost than the cart showed, correct its row.
