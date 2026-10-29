#include "wifi_manager.h"
#include "config.h"
#include <WiFi.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

void WifiManager::init() {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID, AP_PASS, AP_CHANNEL, 0, AP_MAX_CONN);
    Serial.printf("AP started: %s @ %s\n", AP_SSID, WiFi.softAPIP().toString().c_str());
    loadSaved();
}

bool WifiManager::connectSTA(const char* ssid, const char* pass) {
    if (strlen(ssid) == 0) return false;

    Serial.printf("Connecting to %s...\n", ssid);
    WiFi.begin(ssid, pass);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
        delay(250);
    }

    _staConnected = (WiFi.status() == WL_CONNECTED);
    if (_staConnected) {
        strncpy(_ssid, ssid, sizeof(_ssid) - 1);
        strncpy(_pass, pass, sizeof(_pass) - 1);
        Serial.printf("Connected: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("STA connection failed");
    }
    return _staConnected;
}

void WifiManager::loadSaved() {
    File f = LittleFS.open(CONFIG_FILE, "r");
    if (!f) return;

    JsonDocument doc;
    if (deserializeJson(doc, f) == DeserializationError::Ok) {
        const char* ssid = doc["wifi_ssid"];
        const char* pass = doc["wifi_pass"];
        if (ssid && strlen(ssid) > 0) {
            connectSTA(ssid, pass ? pass : "");
        }
    }
    f.close();
}

void WifiManager::saveCredentials(const char* ssid, const char* pass) {
    JsonDocument doc;

    File existing = LittleFS.open(CONFIG_FILE, "r");
    if (existing) {
        deserializeJson(doc, existing);
        existing.close();
    }

    doc["wifi_ssid"] = ssid;
    doc["wifi_pass"] = pass;

    File f = LittleFS.open(CONFIG_FILE, "w");
    if (f) {
        serializeJson(doc, f);
        f.close();
    }
}

String WifiManager::getAPIP() const { return WiFi.softAPIP().toString(); }
String WifiManager::getSTAIP() const { return WiFi.localIP().toString(); }
bool WifiManager::isSTAConnected() const { return WiFi.status() == WL_CONNECTED; }
String WifiManager::getSSID() const { return String(_ssid); }
