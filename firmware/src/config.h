#pragma once

#define FW_VERSION "0.1.1"
#define DEVICE_NAME "DMX-Open"

// --- Hardware pins (ESP32-C3) ---
// C3 has GPIO 0-21 only. GPIO 12/13 are USB-JTAG, avoid those.
#define DMX_TX_PIN    7
#define DMX_RX_PIN    6
#define DMX_DE_PIN    5
#define DMX_UART_NUM  1

#define BTN_PIN       9   // BOOT button on most C3 dev boards
#define LED_PIN       8   // Common LED pin on C3 DevKitM

// --- DMX timing ---
#define DMX_CHANNELS   512
#define DMX_FPS         40
#define DMX_BREAK_US   176
#define DMX_MAB_US      16

// --- WiFi defaults ---
#define AP_SSID     "DMX-Open"
#define AP_PASS     "dmxopen1"
#define AP_CHANNEL   1
#define AP_MAX_CONN  4

// --- Web server ---
#define HTTP_PORT    80

// --- LittleFS paths ---
#define WWW_DIR      "/www"
#define SHOWS_DIR    "/shows"
#define CONFIG_FILE  "/config.json"
