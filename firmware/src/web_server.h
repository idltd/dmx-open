#pragma once
#include <ESPAsyncWebServer.h>
#include "dmx_output.h"
#include "show_player.h"
#include "wifi_manager.h"

class WebServer {
public:
    void init(DmxOutput& dmx, ShowPlayer& player, WifiManager& wifi);
    void broadcastDmxState();
    void broadcastPlayerState();

private:
    AsyncWebServer  _http{80};
    AsyncWebSocket  _ws{"/ws"};
    DmxOutput*      _dmx    = nullptr;
    ShowPlayer*     _player = nullptr;
    WifiManager*    _wifi   = nullptr;
    unsigned long   _lastBroadcast = 0;

    void setupRoutes();
    void setupWebSocket();
    void handleWsMessage(AsyncWebSocketClient* client, uint8_t* data, size_t len);

    // API handlers
    void apiGetStatus(AsyncWebServerRequest* req);
    void apiGetDmx(AsyncWebServerRequest* req);
    void apiPutDmx(AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total);
    void apiBlackout(AsyncWebServerRequest* req);
    void apiGetShows(AsyncWebServerRequest* req);
    void apiUploadShow(AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total);
    void apiDeleteShow(AsyncWebServerRequest* req);
    void apiPlayerAction(AsyncWebServerRequest* req);
    void apiGetConfig(AsyncWebServerRequest* req);
    void apiPutConfig(AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total);
};
