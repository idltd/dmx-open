# DMX-Open — Bill of Materials

## Required

| Part | Approx. Cost | Notes |
|---|---|---|
| ESP32 DevKit (any) | $5–8 | ESP32-WROOM-32 or ESP32-S3. Any generic dev board works. |
| ADM2582E breakout | $4–6 | Isolated RS-485 transceiver with built-in DC-DC. Search AliExpress/eBay for "ADM2582E module". Alternatively: ISO3082 module. |
| XLR 5-pin female | $1–2 | Panel mount. DMX standard is 5-pin; 3-pin also common. |
| USB cable + 5V supply | $0–3 | Powers the ESP32. USB phone charger works. |

**Total: ~$10–18**

## Wiring

```
ESP32                ADM2582E               XLR 5-pin
─────                ────────               ─────────
GPIO 17 (TX) ──────► DI
GPIO 16 (RX) ◄────── RO
GPIO  4      ──────► DE + RE (tie together)
3.3V         ──────► VCC1 (logic side)
GND          ──────► GND1 (logic side)

                     VCC2 ◄── 5V (from USB/separate)    
                     GND2 ──────────────────► Pin 1 (Ground)
                     A    ──────────────────► Pin 3 (Data+)
                     B    ──────────────────► Pin 2 (Data-)
```

Pin 4 and 5 on XLR are unused (or second data pair if needed later).

## Notes

- The ADM2582E provides **galvanic isolation** between the ESP32 and the DMX bus.
  This protects both sides from ground loops and voltage spikes — essential for
  real-world use where DMX cables run alongside power cables.
- The DE (Driver Enable) and RE (Receiver Enable, active low) pins are tied together.
  When GPIO 4 is HIGH, the chip transmits. When LOW, it receives. For TX-only
  operation, you can tie DE high and RE high (disabling receiver).
- If using a non-isolated transceiver (MAX485, SN75176): wire the same way but
  connect VCC to 3.3V and share ground. Works fine for bench testing but not
  recommended for gigs.

## Optional

| Part | Notes |
|---|---|
| 3D-printed enclosure | STL files TBD — small box with XLR + USB cutouts |
| Momentary push button | If your board's BOOT button isn't accessible in the enclosure |
| Status LED | Most ESP32 boards have one on GPIO 2 already |
| SD card module | For larger show storage (future) |
