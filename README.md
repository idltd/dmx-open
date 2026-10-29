# DMX-Open

Open-source DMX512 lighting controller. An ESP32 + isolated RS-485 transceiver in
a small box — all control via a web browser on your phone, tablet, or laptop.

**Two modes:**
- **Programmed playback** — load a show file onto the device, trigger it with a
  button press or API call. The box runs the show autonomously.
- **Live interactive** — touch-friendly fader bank in your browser. Real-time
  control over WebSocket.

## Hardware

~$10–18 in parts. See [hardware/BOM.md](hardware/BOM.md) for the full list and wiring.

- ESP32 dev board (any)
- ADM2582E isolated RS-485 module
- 5-pin XLR connector

## Quick Start

1. Wire up the hardware per the BOM
2. Install [PlatformIO](https://platformio.org/)
3. Run `00-startup.bat` to build and flash
4. Connect to WiFi network **DMX-Open** (password: `dmxopen1`)
5. Open http://192.168.4.1 in your browser

## Architecture

The ESP32 runs DMX output on a dedicated core at 40fps for rock-solid timing.
The web server, WebSocket, and REST API run on the other core. The PWA is served
from the device's flash filesystem — no internet needed.

See [docs/architecture.md](docs/architecture.md) for the full spec: API contract,
WebSocket protocol, show file format, and PWA structure.

## Show Files

JSON-based step sequences with per-channel values and crossfades:

```json
{
    "name": "my-show",
    "loop": true,
    "steps": [
        {"t": 0,    "fade": 0,    "ch": [[1, 255], [2, 128]]},
        {"t": 5000, "fade": 2000, "ch": [[1, 0],   [3, 255]]}
    ]
}
```

Create shows in the browser editor, or write them by hand and upload via the API.

## Project Status

**v0.1.0** — Initial release. Core platform:
- [x] DMX512 output with galvanic isolation
- [x] WiFi AP mode (direct connection, no router)
- [x] Optional STA mode (join existing network)
- [x] REST API for full programmatic control
- [x] WebSocket for real-time state
- [x] Show file storage and playback engine
- [x] Autoplay on boot
- [x] Physical button: short press play/pause, long press stop+blackout
- [x] PWA with live faders, show manager, step editor, settings
- [x] Service worker for offline caching

### Roadmap
- [ ] Fixture profiles (RGB, RGBW, moving head) with named parameters
- [ ] Scene snapshots (capture live state as a step)
- [ ] Art-Net input (receive from external consoles)
- [ ] Multi-universe (multiple DMX outputs)
- [ ] BPM sync / tap tempo for chase effects
- [ ] OTA firmware updates via the web UI
- [ ] 3D-printable enclosure STL files

## License

MIT
