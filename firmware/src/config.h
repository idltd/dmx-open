#pragma once

#define FW_VERSION "0.1.0"
#define DEVICE_NAME "DMX-Open"

// --- Hardware pins (ESP32 DevKit generic) ---
#define DMX_TX_PIN   17
#define DMX_RX_PIN   16
#define DMX_DE_PIN    4
#define DMX_UART_NUM  2

#define BTN_PIN       0   // BOOT button on most dev boards
#define LED_PIN       2   // Built-in LED

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
