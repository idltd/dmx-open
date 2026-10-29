#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "config.h"
#include "dmx_output.h"
#include "show_player.h"
#include "wifi_manager.h"
#include "web_server.h"

DmxOutput   dmx;
ShowPlayer  player;
WifiManager wifi;
WebServer   web;

static bool btnDown = false;
static unsigned long btnDownMs = 0;
static unsigned long lastPlayerBroadcast = 0;

// DMX output runs on core 1 for consistent timing
void dmxTask(void* param) {
    TickType_t interval = pdMS_TO_TICKS(1000 / DMX_FPS);
    TickType_t lastWake = xTaskGetTickCount();

    for (;;) {
        player.tick(const_cast<uint8_t*>(dmx.getRaw()));
        dmx.sendFrame();
        vTaskDelayUntil(&lastWake, interval);
    }
}

void setup() {
    Serial.begin(115200);
    Serial.printf("\n%s v%s\n", DEVICE_NAME, FW_VERSION);

    pinMode(BTN_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS mount failed");
        return;
    }

    // Ensure shows directory exists
    if (!LittleFS.exists(SHOWS_DIR)) LittleFS.mkdir(SHOWS_DIR);

    dmx.init();
    wifi.init();
    web.init(dmx, player, wifi);

    // Autoplay if configured
    File cfg = LittleFS.open(CONFIG_FILE, "r");
    if (cfg) {
        JsonDocument doc;
        if (deserializeJson(doc, cfg) == DeserializationError::Ok) {
            const char* autoplay = doc["autoplay"];
            if (autoplay && strlen(autoplay) > 0) {
                Serial.printf("Autoplay: %s\n", autoplay);
                if (player.load(autoplay)) {
                    player.play();
                    digitalWrite(LED_PIN, HIGH);
                }
            }
        }
        cfg.close();
    }

    xTaskCreatePinnedToCore(dmxTask, "DMX", 4096, NULL, 1, NULL, 1);
    Serial.println("Ready");
}

void loop() {
    // Button: short press = play/pause, long press (>1s) = stop + blackout
    bool pressed = digitalRead(BTN_PIN) == LOW;
    if (pressed && !btnDown) {
        btnDown = true;
        btnDownMs = millis();
    }
    if (!pressed && btnDown) {
        btnDown = false;
        unsigned long held = millis() - btnDownMs;
        if (held > 1000) {
            player.stop();
            dmx.blackout();
            digitalWrite(LED_PIN, LOW);
        } else {
            if (player.isPlaying()) {
                player.pause();
            } else if (player.isPaused()) {
                player.play();
            } else if (player.isLoaded()) {
                player.play();
                digitalWrite(LED_PIN, HIGH);
            }
        }
        web.broadcastPlayerState();
        web.broadcastDmxState();
    }

    // LED: solid = playing, blink = paused, off = stopped
    if (player.isPlaying()) {
        digitalWrite(LED_PIN, HIGH);
    } else if (player.isPaused()) {
        digitalWrite(LED_PIN, (millis() / 500) % 2);
    } else {
        digitalWrite(LED_PIN, LOW);
    }

    // Periodic player state broadcast while playing
    if (player.isPlaying() && millis() - lastPlayerBroadcast > 500) {
        lastPlayerBroadcast = millis();
        web.broadcastPlayerState();
    }

    delay(10);
}
