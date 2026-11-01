# Architecture

## Overview

DMX-Open is a two-part system:

1. **Firmware** (ESP32-C3, C++) — handles WiFi, DMX output, show playback, and serves
   the web UI. Runs autonomously once a show is loaded.
2. **PWA** (HTML/CSS/JS) — served from the ESP32-C3's flash filesystem. Provides live
   control, show editing, and device configuration via any browser.

The ESP32-C3 is the only hardware. No Raspberry Pi, no laptop, no cloud. Connect to
its WiFi, open the browser, control your lights.

## Firmware Architecture

```
Single core (RISC-V)
┌─────────────────────────────────────────┐
│  WiFi (AP + STA)                        │
│  HTTP Server / WebSocket / REST API     │
│  Button/LED                             │
│  main loop()                            │
│                                         │
│  Show Player  →  DMX Output             │
│    tick()          sendFrame()           │
│                    UART → RS-485        │
│                    40 fps (FreeRTOS task)│
└─────────────────────────────────────────┘
```

The DMX task runs at 40fps with `vTaskDelayUntil` for consistent timing.
The channel array (512 bytes) is shared between the main loop and the DMX
task — single-byte writes are atomic, so no mutex is needed.

## Communication

### WebSocket (`/ws`)

Real-time bidirectional. Client sends commands, server pushes state.

**Client → Server:**
```json
{"cmd": "set", "ch": {"1": 255, "5": 128}}
{"cmd": "blackout"}
{"cmd": "master", "value": 200}
{"cmd": "play", "show": "demo"}
{"cmd": "pause"}
{"cmd": "stop"}
{"cmd": "getState"}
```

**Server → Client:**
```json
{"type": "dmx", "ch": [0,0,255,...], "master": 255}
{"type": "player", "loaded": true, "playing": true, "paused": false,
 "show": "demo", "pos": 5230, "duration": 15000, "loop": true}
```

### REST API

For non-real-time operations.

| Method | Path | Body | Purpose |
|--------|------|------|---------|
| GET | `/api/status` | — | Device info, IPs, heap, uptime |
| GET | `/api/dmx` | — | Current 512 channels + master |
| PUT | `/api/dmx` | `{"ch":{"1":255},"master":200}` | Set channels |
| POST | `/api/blackout` | — | All channels to 0 |
| GET | `/api/shows` | — | List stored shows |
| POST | `/api/shows` | Show JSON | Upload/save a show |
| DELETE | `/api/shows?name=x` | — | Delete a show |
| POST | `/api/player/play?show=x` | — | Start playback |
| POST | `/api/player/pause` | — | Pause |
| POST | `/api/player/stop` | — | Stop |
| GET | `/api/config` | — | Read device config |
| PUT | `/api/config` | `{"wifi_ssid":"...","autoplay":"..."}` | Update config |

## Show File Format

JSON stored in `/shows/<name>.json` on LittleFS.

```json
{
    "name": "my-show",
    "v": 1,
    "loop": true,
    "steps": [
        {"t": 0,    "fade": 0,    "ch": [[1, 255], [2, 128]]},
        {"t": 5000, "fade": 2000, "ch": [[1, 0],   [3, 255]]},
        {"t": 8000, "fade": 1000, "ch": [[1, 255], [2, 255], [3, 255]]}
    ]
}
```

- `t` — time in ms from show start when this step triggers
- `fade` — crossfade duration in ms (0 = snap)
- `ch` — array of `[channel, value]` pairs (channel 1–512, value 0–255)
- Channels not mentioned in a step hold their current value
- Duration is implicit: last step's `t` + last step's `fade`

### Playback Engine

When a step triggers:
1. Snapshot current output as "from" state
2. Apply the step's channel changes to a "to" state
3. Linearly interpolate between "from" and "to" over the fade duration
4. A new step interrupts any in-progress fade from its current position

## PWA Structure

Single-page app with hash routing. No build step, no framework, no dependencies.

| View | Hash | Purpose |
|------|------|---------|
| Live | `#live` | Fader bank with touch-friendly vertical faders, master, bank switching |
| Shows | `#shows` | List stored shows, play/edit/delete |
| Editor | `#editor` | Build shows: add steps, set times/fades, pick channels |
| Settings | `#settings` | WiFi config, autoplay, device info |

The PWA registers a service worker for offline caching. Once loaded, it works even
if WiFi drops momentarily — the WebSocket reconnects automatically.

## File Layout

```
firmware/
├── platformio.ini
├── src/
│   ├── config.h          Pin defs, constants
│   ├── main.cpp          Setup, loop, DMX task
│   ├── dmx_output.h/cpp  UART-based DMX512 output
│   ├── show_player.h/cpp Show file parser + playback engine
│   ├── web_server.h/cpp  HTTP + WebSocket + REST API
│   └── wifi_manager.h/cpp WiFi AP/STA
└── data/
    ├── www/              PWA (uploaded to LittleFS)
    │   ├── index.html
    │   ├── app.js
    │   ├── style.css
    │   ├── manifest.json
    │   └── sw.js
    └── shows/
        └── demo.json
```
