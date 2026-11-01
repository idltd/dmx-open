# Fixture Profiles

## LED Φ10 PAR36 (RGB PAR Can)

Cheap RGB LED PAR can. 4 DMX channels per fixture.

### DMX Channel Map (4 channels)

| Offset | Function | Range |
|--------|----------|-------|
| +0 | Dimmer / Strobe | 0 = off, 1–127 = brightness, 128–254 = strobe (slow→fast), 255 = full on |
| +1 | Red | 0–255 |
| +2 | Green | 1–255 (note: 0 may not fully extinguish on some units) |
| +3 | Blue | 0–255 |

### DIP Switch Addressing

10-position DIP switch on each fixture.

- **Switch 10** must be ON for DMX mode.
- **Switches 1–9** set the start address in binary:

| Switch | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |
|--------|---|---|---|---|---|---|---|---|---|
| Value  | 1 | 2 | 4 | 8 | 16| 32| 64|128|256|

### Recommended Addressing (4 fixtures)

| Fixture | Start Address | DIP 1-9 (ON switches) |
|---------|---------------|----------------------|
| 1 | 1 | 1 |
| 2 | 5 | 1, 3 |
| 3 | 9 | 1, 4 |
| 4 | 13 | 1, 3, 4 |

### Technical Specs

- 60 LED points: 18 red, 21 green, 21 blue
- Beam angle: 15–60° (spot/astigmatism)
- Brightness: R 8000mcd, G 20000mcd, B 6000mcd
- Power: 8W
- Sound-reactive mode available (standalone, not via DMX)
- Built-in chase programs (standalone, switches 1–3 select pattern)
