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
| 2026-11-17 | Amazon | 2 Bridgold LED kits (10 TSAL6200 + 10 receivers) | $17.98 | $0.00 | $1.80 | $19.78 | Board batch (5) |
| 2026-11-13 | JLC3DP | Enclosure test piece (7 parts) | $10.79 | $31.65 | $1.04 | $43.48 | Development (case) |

JLCPCB's duties and taxes are customs $58.07, disbursement fee $7.52 and sales tax $16.58. The DigiKey order's shipping ($8.49), tariff ($3.73) and sales tax ($3.29) total $44.67 with the $29.16 of merchandise, split between its two lines by merchandise price.

## Summary

**Board batch, 5 boards** (landed):

| | Batch | Per board |
|---|---|---|
| JLCPCB, PCB and assembly | $285.98 | $57.20 |
| DigiKey, hand-fitted parts (with spares) | $38.91 | $7.78 |
| Amazon, IR LED kits (10 LEDs, 10 spare receivers) | $19.78 | $3.96 |
| **Electronics** | **$344.67** | **$68.93** |
| Batteries | $63.40 | $12.68 |
| **Electronics plus battery** | **$408.07** | **$81.61** |

**Case:** no case has been bought yet, so there is no per-case figure (see the estimate below). The test piece ($43.48 landed, of which $31.65 was shipping on $10.79 of parts) is development, not a case. Shipping dominates small JLC3DP orders, so a case order's cost per case will fall a lot if several cases, or the case with spare caps and slider knobs, go in one shipment. Record each case print order as a row above, then divide by the cases it makes.

**Development, not spread over the units:** IR sink-test parts $5.76; the test piece $43.48 (total $49.24).

**One-time costs inside the JLCPCB figure** (the same for 5 boards or 50): setup $25.75, stencil $8.27, engineering $4.00, feeder loading $52.70, together $90.72. A larger batch spreads them, and a second batch would repeat them.

## Estimates (not spending)

Kept apart from the ledger and the summary above; none of it is counted there. Replace it with real figures when a case is ordered.

**Printing one case in MJF nylon at JLC3DP** (estimated 2026-11-17). The test piece's seven parts fit a simple price rule: about **$0.34 per cm³ with a $0.95 minimum per part** (plate 12.9 cm³ for $4.37, key base 4.9 cm³ for $1.67; the key face and caps sit at the minimum). Volumes come from the case model.

| Part | Volume | Estimated price |
|---|---|---|
| Bottom shell | 23.2 cm³ | $7.89 |
| Top shell | 20.6 cm³ | $7.00 |
| 7 keycaps (each at the $0.95 minimum) | 0.3–1.0 cm³ each | $6.65 |
| A/B slider | 0.3 cm³ | $0.95 |
| **Printing, one case** | | **about $22.50** |

| Delivered | Assumption | Estimate |
|---|---|---|
| One case | shipping $32–40, tax about 2.4 % (as on the test piece) | about **$56–64** |
| Five cases, one shipment | shipping $45–60 | about **$32–35 each** |

**Possible saving:** the caps and slider total about 3.9 cm³, or roughly $1.30 by volume, but cost $7.60 as eight separate parts at the minimum. Sending them as one file would bring one case's printing to about $16, a saving of about $6 per case (about $26–29 per case delivered in a batch of five). Whether JLC3DP accepts several loose bodies in one file hasn't been tried.

**Not in these numbers:** finishing (dyed-black caps and slider; JLC3DP's dye pricing not checked), and any volume discount or size effect on the shells, which are larger than anything in the test piece. For an exact figure, upload the STLs to JLC3DP's quote page without ordering. The shipping figures are guesses from the test piece ($31.65 for 21 cm³).

## Updating this file

When an order is placed, add a row with its merchandise, shipping, duties and tax, and what it's charged to, then update the summary. When an order arrives with a different landed cost than the cart showed, correct its row.
