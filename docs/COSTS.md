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
| 2026-11-15 | Amazon | M2/M2.5/M3 pan-head self-tapping screws, 1200-piece assortment | $8.49 | $0.00 | $0.85 | $9.34 | Case batch (5), hardware |
| 2026-11-13 | DigiKey | Key switches TS02-66-130-BK-160-SCR-D ×100 | $10.33 | $8.49 | $3.27 | $22.09 | Board batch (5) |
| 2026-11-13 | JLC3DP | Enclosure test piece (7 parts) | $10.79 | $31.65 | $23.08 | $65.52 | Development (case) |

The key-switch order's duties and tax are a $2.03 tariff and $1.24 sales tax. Its 100 switches cover the 5 boards (8 each) with 60 spare, and one went into the test piece's key fixture; as spares, all of it is charged to the batch.

The test piece's duties and tax are the $1.04 sales tax paid at checkout plus a $22.04 import duty that DHL billed on arrival (paid 2026-11-20, John). The checkout total ($43.48) didn't include it.

JLCPCB's duties and taxes are customs $58.07, disbursement fee $7.52 and sales tax $16.58. The DigiKey order's shipping ($8.49), tariff ($3.73) and sales tax ($3.29) total $44.67 with the $29.16 of merchandise, split between its two lines by merchandise price.

## Summary

**Board batch, 5 boards** (landed):

| | Batch | Per board |
|---|---|---|
| JLCPCB, PCB and assembly | $285.98 | $57.20 |
| DigiKey, hand-fitted parts (with spares) | $38.91 | $7.78 |
| Amazon, IR LED kits (10 LEDs, 10 spare receivers) | $19.78 | $3.96 |
| DigiKey, key switches (100, with spares) | $22.09 | $4.42 |
| **Electronics** | **$366.76** | **$73.35** |
| Batteries | $63.40 | $12.68 |
| **Electronics plus battery** | **$430.16** | **$86.03** |

**Case:** no case print has been ordered yet, so there is no per-case figure (see the estimate below). Hardware bought so far: the screw assortment, $9.34 landed, charged to the case batch (each case uses 4 board screws and 4 case screws, so the assortment covers many more than five; as spares, all of it counts). Proposed charge (Claude); change it if the screws should count as development stock. The test piece ($65.52 landed: $10.79 of parts, $31.65 shipping, $23.08 duties and tax) is development, not a case. Shipping dominates small JLC3DP orders, so a case order's cost per case will fall a lot if several cases, or the case with spare caps and slider knobs, go in one shipment. Record each case print order as a row above, then divide by the cases it makes.

**Development, not spread over the units:** IR sink-test parts $5.76; the test piece $65.52 (total $71.28).

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

**Not in these numbers:** finishing (black dye on the two shells only, per the finish decided 2026-11-21 by John: black shells, keycaps and slider left natural grey; JLC3DP's dye pricing not checked), and any volume discount or size effect on the shells, which are larger than anything in the test piece. For an exact figure, upload the STLs to JLC3DP's quote page without ordering. The shipping figures are guesses from the test piece ($31.65 for 21 cm³). **The delivered estimates above are too low:** they assumed only the checkout tax, but the test piece also drew a $22.04 import duty from DHL on arrival, about twice its merchandise value. How that duty scales with order value is unknown until a second shipment arrives; budget roughly $20–25 extra per shipment for now.

## Updating this file

When an order is placed, add a row with its merchandise, shipping, duties and tax, and what it's charged to, then update the summary. When an order arrives with a different landed cost than the cart showed, correct its row.
