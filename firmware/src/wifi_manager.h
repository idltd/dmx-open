#pragma once
#include <Arduino.h>

class WifiManager {
public:
    void init();
    bool connectSTA(const char* ssid, const char* pass);
    void loadSaved();
    void saveCredentials(const char* ssid, const char* pass);
    String getAPIP() const;
    String getSTAIP() const;
    bool isSTAConnected() const;
    String getSSID() const;

private:
    char _ssid[64] = {0};
    char _pass[64] = {0};
    bool _staConnected = false;
};
