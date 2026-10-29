#include "web_server.h"
#include "config.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

static String showBodyBuf;

void WebServer::init(DmxOutput& dmx, ShowPlayer& player, WifiManager& wifi) {
    _dmx    = &dmx;
    _player = &player;
    _wifi   = &wifi;

    setupRoutes();
    setupWebSocket();
    _http.addHandler(&_ws);
    _http.serveStatic("/", LittleFS, WWW_DIR).setDefaultFile("index.html");
    _http.begin();
    Serial.printf("HTTP server on port %d\n", HTTP_PORT);
}

// ── WebSocket ──────────────────────────────────────────────

void WebServer::setupWebSocket() {
    _ws.onEvent([this](AsyncWebSocket* ws, AsyncWebSocketClient* client,
                       AwsEventType type, void* arg, uint8_t* data, size_t len) {
        if (type == WS_EVT_CONNECT) {
            broadcastDmxState();
            broadcastPlayerState();
        } else if (type == WS_EVT_DATA) {
            handleWsMessage(client, data, len);
        }
    });
}

void WebServer::handleWsMessage(AsyncWebSocketClient* client, uint8_t* data, size_t len) {
    JsonDocument doc;
    if (deserializeJson(doc, data, len) != DeserializationError::Ok) return;

    const char* cmd = doc["cmd"];
    if (!cmd) return;

    if (strcmp(cmd, "set") == 0) {
        JsonObject ch = doc["ch"].as<JsonObject>();
        if (ch) {
            for (JsonPair p : ch) {
                uint16_t channel = atoi(p.key().c_str());
                uint8_t value = p.value().as<uint8_t>();
                _dmx->setChannel(channel, value);
            }
        }
        broadcastDmxState();
    }
    else if (strcmp(cmd, "blackout") == 0) {
        _dmx->blackout();
        broadcastDmxState();
    }
    else if (strcmp(cmd, "master") == 0) {
        _dmx->setMaster(doc["value"] | 255);
        broadcastDmxState();
    }
    else if (strcmp(cmd, "play") == 0) {
        const char* show = doc["show"];
        if (show) {
            if (_player->load(show)) _player->play();
        } else if (_player->isLoaded()) {
            _player->play();
        }
        broadcastPlayerState();
    }
    else if (strcmp(cmd, "pause") == 0) {
        _player->pause();
        broadcastPlayerState();
    }
    else if (strcmp(cmd, "stop") == 0) {
        _player->stop();
        broadcastPlayerState();
    }
    else if (strcmp(cmd, "getState") == 0) {
        broadcastDmxState();
        broadcastPlayerState();
    }
}

void WebServer::broadcastDmxState() {
    if (_ws.count() == 0) return;

    JsonDocument doc;
    doc["type"] = "dmx";
    doc["master"] = _dmx->getMaster();
    JsonArray ch = doc["ch"].to<JsonArray>();
    const uint8_t* raw = _dmx->getRaw();
    for (int i = 0; i < DMX_CHANNELS; i++) ch.add(raw[i]);

    String out;
    serializeJson(doc, out);
    _ws.textAll(out);
}

void WebServer::broadcastPlayerState() {
    if (_ws.count() == 0) return;

    JsonDocument doc;
    doc["type"]     = "player";
    doc["loaded"]   = _player->isLoaded();
    doc["playing"]  = _player->isPlaying();
    doc["paused"]   = _player->isPaused();
    doc["show"]     = _player->name();
    doc["pos"]      = _player->position();
    doc["duration"] = _player->duration();
    doc["loop"]     = _player->loops();

    String out;
    serializeJson(doc, out);
    _ws.textAll(out);
}

// ── REST API ───────────────────────────────────────────────

void WebServer::setupRoutes() {
    _http.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* r) { apiGetStatus(r); });
    _http.on("/api/dmx", HTTP_GET, [this](AsyncWebServerRequest* r) { apiGetDmx(r); });
    _http.on("/api/blackout", HTTP_POST, [this](AsyncWebServerRequest* r) { apiBlackout(r); });
    _http.on("/api/shows", HTTP_GET, [this](AsyncWebServerRequest* r) { apiGetShows(r); });
    _http.on("/api/player/play", HTTP_POST, [this](AsyncWebServerRequest* r) { apiPlayerAction(r); });
    _http.on("/api/player/pause", HTTP_POST, [this](AsyncWebServerRequest* r) { apiPlayerAction(r); });
    _http.on("/api/player/stop", HTTP_POST, [this](AsyncWebServerRequest* r) { apiPlayerAction(r); });

    // PUT /api/dmx — set channels via JSON body
    _http.on("/api/dmx", HTTP_PUT,
        [](AsyncWebServerRequest* r) { r->send(200); },
        nullptr,
        [this](AsyncWebServerRequest* r, uint8_t* d, size_t l, size_t i, size_t t) {
            apiPutDmx(r, d, l, i, t);
        });

    // POST /api/shows/:name — upload show JSON
    _http.on("/api/shows", HTTP_POST,
        [](AsyncWebServerRequest* r) {},
        nullptr,
        [this](AsyncWebServerRequest* r, uint8_t* d, size_t l, size_t i, size_t t) {
            apiUploadShow(r, d, l, i, t);
        });

    // DELETE /api/shows?name=xxx
    _http.on("/api/shows", HTTP_DELETE, [this](AsyncWebServerRequest* r) { apiDeleteShow(r); });

    // GET /api/config
    _http.on("/api/config", HTTP_GET, [this](AsyncWebServerRequest* r) { apiGetConfig(r); });

    // PUT /api/config
    _http.on("/api/config", HTTP_PUT,
        [](AsyncWebServerRequest* r) { r->send(200); },
        nullptr,
        [this](AsyncWebServerRequest* r, uint8_t* d, size_t l, size_t i, size_t t) {
            apiPutConfig(r, d, l, i, t);
        });
}

void WebServer::apiGetStatus(AsyncWebServerRequest* req) {
    JsonDocument doc;
    doc["version"]  = FW_VERSION;
    doc["device"]   = DEVICE_NAME;
    doc["ap_ip"]    = _wifi->getAPIP();
    doc["sta_ip"]   = _wifi->getSTAIP();
    doc["sta_connected"] = _wifi->isSTAConnected();
    doc["sta_ssid"] = _wifi->getSSID();
    doc["heap"]     = ESP.getFreeHeap();
    doc["uptime"]   = millis() / 1000;

    String out;
    serializeJson(doc, out);
    req->send(200, "application/json", out);
}

void WebServer::apiGetDmx(AsyncWebServerRequest* req) {
    JsonDocument doc;
    doc["master"] = _dmx->getMaster();
    JsonArray ch = doc["ch"].to<JsonArray>();
    const uint8_t* raw = _dmx->getRaw();
    for (int i = 0; i < DMX_CHANNELS; i++) ch.add(raw[i]);

    String out;
    serializeJson(doc, out);
    req->send(200, "application/json", out);
}

void WebServer::apiPutDmx(AsyncWebServerRequest* req, uint8_t* data, size_t len,
                           size_t index, size_t total) {
    JsonDocument doc;
    if (deserializeJson(doc, data, len) != DeserializationError::Ok) return;

    if (doc["master"].is<uint8_t>()) _dmx->setMaster(doc["master"]);

    JsonObject ch = doc["ch"].as<JsonObject>();
    if (ch) {
        for (JsonPair p : ch) {
            _dmx->setChannel(atoi(p.key().c_str()), p.value().as<uint8_t>());
        }
    }
    broadcastDmxState();
}

void WebServer::apiBlackout(AsyncWebServerRequest* req) {
    _dmx->blackout();
    broadcastDmxState();
    req->send(200, "application/json", "{\"ok\":true}");
}

void WebServer::apiGetShows(AsyncWebServerRequest* req) {
    JsonDocument doc;
    JsonArray list = doc.to<JsonArray>();

    File dir = LittleFS.open(SHOWS_DIR);
    if (dir && dir.isDirectory()) {
        File f = dir.openNextFile();
        while (f) {
            if (!f.isDirectory()) {
                JsonObject show = list.add<JsonObject>();
                String fname = f.name();
                if (fname.startsWith("/")) fname = fname.substring(1);
                // Strip .json extension for display name
                if (fname.endsWith(".json")) fname = fname.substring(0, fname.length() - 5);
                show["name"] = fname;
                show["size"] = f.size();
            }
            f = dir.openNextFile();
        }
    }

    String out;
    serializeJson(doc, out);
    req->send(200, "application/json", out);
}

void WebServer::apiUploadShow(AsyncWebServerRequest* req, uint8_t* data, size_t len,
                               size_t index, size_t total) {
    if (index == 0) showBodyBuf = "";
    showBodyBuf += String((char*)data, len);

    if (index + len >= total) {
        JsonDocument doc;
        if (deserializeJson(doc, showBodyBuf) != DeserializationError::Ok) {
            req->send(400, "application/json", "{\"error\":\"invalid json\"}");
            showBodyBuf = "";
            return;
        }

        const char* name = doc["name"];
        if (!name || strlen(name) == 0) {
            req->send(400, "application/json", "{\"error\":\"name required\"}");
            showBodyBuf = "";
            return;
        }

        String path = String(SHOWS_DIR) + "/" + name + ".json";
        File f = LittleFS.open(path, "w");
        if (f) {
            f.print(showBodyBuf);
            f.close();
            req->send(200, "application/json", "{\"ok\":true}");
        } else {
            req->send(500, "application/json", "{\"error\":\"write failed\"}");
        }
        showBodyBuf = "";
    }
}

void WebServer::apiDeleteShow(AsyncWebServerRequest* req) {
    if (!req->hasParam("name")) {
        req->send(400, "application/json", "{\"error\":\"name required\"}");
        return;
    }
    String name = req->getParam("name")->value();
    String path = String(SHOWS_DIR) + "/" + name + ".json";

    if (LittleFS.remove(path)) {
        req->send(200, "application/json", "{\"ok\":true}");
    } else {
        req->send(404, "application/json", "{\"error\":\"not found\"}");
    }
}

void WebServer::apiPlayerAction(AsyncWebServerRequest* req) {
    String url = req->url();

    if (url.endsWith("/play")) {
        if (req->hasParam("show")) {
            String show = req->getParam("show")->value();
            if (_player->load(show.c_str())) _player->play();
            else { req->send(404, "application/json", "{\"error\":\"show not found\"}"); return; }
        } else if (_player->isLoaded()) {
            _player->play();
        } else {
            req->send(400, "application/json", "{\"error\":\"no show loaded\"}");
            return;
        }
    }
    else if (url.endsWith("/pause")) { _player->pause(); }
    else if (url.endsWith("/stop"))  { _player->stop(); }

    broadcastPlayerState();
    req->send(200, "application/json", "{\"ok\":true}");
}

void WebServer::apiGetConfig(AsyncWebServerRequest* req) {
    File f = LittleFS.open(CONFIG_FILE, "r");
    if (!f) {
        req->send(200, "application/json", "{}");
        return;
    }
    String content = f.readString();
    f.close();
    req->send(200, "application/json", content);
}

void WebServer::apiPutConfig(AsyncWebServerRequest* req, uint8_t* data, size_t len,
                              size_t index, size_t total) {
    JsonDocument doc;
    if (deserializeJson(doc, data, len) != DeserializationError::Ok) return;

    if (doc["wifi_ssid"].is<const char*>()) {
        const char* ssid = doc["wifi_ssid"];
        const char* pass = doc["wifi_pass"] | "";
        _wifi->saveCredentials(ssid, pass);
        _wifi->connectSTA(ssid, pass);
    }

    if (doc["autoplay"].is<const char*>()) {
        JsonDocument cfg;
        File existing = LittleFS.open(CONFIG_FILE, "r");
        if (existing) { deserializeJson(cfg, existing); existing.close(); }
        cfg["autoplay"] = doc["autoplay"];
        File f = LittleFS.open(CONFIG_FILE, "w");
        if (f) { serializeJson(cfg, f); f.close(); }
    }
}
