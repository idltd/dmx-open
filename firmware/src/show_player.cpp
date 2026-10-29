#include "show_player.h"
#include "config.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

bool ShowPlayer::load(const char* name) {
    unload();

    String path = String(SHOWS_DIR) + "/" + name + ".json";
    File f = LittleFS.open(path, "r");
    if (!f) return false;

    JsonDocument doc;
    if (deserializeJson(doc, f) != DeserializationError::Ok) {
        f.close();
        return false;
    }
    f.close();

    JsonArray steps = doc["steps"].as<JsonArray>();
    if (!steps || steps.size() == 0) return false;

    _stepCount = steps.size();
    _steps = new ShowStep[_stepCount];

    uint16_t totalCh = 0;
    for (JsonVariant s : steps) {
        JsonArray ch = s["ch"].as<JsonArray>();
        if (ch) totalCh += ch.size();
    }

    _chVals = new ChVal[totalCh];
    _chValCount = totalCh;

    uint16_t chIdx = 0;
    for (uint16_t i = 0; i < _stepCount; i++) {
        JsonObject s = steps[i];
        _steps[i].time    = s["t"] | (uint32_t)0;
        _steps[i].fade    = s["fade"] | (uint16_t)0;
        _steps[i].firstCh = chIdx;
        _steps[i].numCh   = 0;

        JsonArray ch = s["ch"].as<JsonArray>();
        if (ch) {
            for (JsonVariant cv : ch) {
                JsonArray pair = cv.as<JsonArray>();
                if (pair && pair.size() >= 2) {
                    _chVals[chIdx].ch  = pair[0].as<uint16_t>();
                    _chVals[chIdx].val = pair[1].as<uint8_t>();
                    chIdx++;
                    _steps[i].numCh++;
                }
            }
        }
    }

    _loop = doc["loop"] | false;

    // Duration = last step time + last step fade
    ShowStep& last = _steps[_stepCount - 1];
    _duration = last.time + last.fade;

    strncpy(_name, name, sizeof(_name) - 1);
    _loaded = true;
    return true;
}

void ShowPlayer::unload() {
    delete[] _steps;  _steps = nullptr;
    delete[] _chVals; _chVals = nullptr;
    _stepCount = _chValCount = 0;
    _loaded = _playing = _paused = false;
    _activeStep = 0xFFFF;
    _name[0] = 0;
}

void ShowPlayer::play() {
    if (!_loaded) return;
    if (_paused) {
        _startMs += millis() - _pauseMs;
        _fadeStartMs += millis() - _pauseMs;
        _paused = false;
        return;
    }
    _playing = true;
    _paused = false;
    _startMs = millis();
    _activeStep = 0xFFFF;
    memset(_from, 0, 512);
    memset(_to, 0, 512);
}

void ShowPlayer::pause() {
    if (_playing && !_paused) {
        _paused = true;
        _pauseMs = millis();
    }
}

void ShowPlayer::stop() {
    _playing = false;
    _paused = false;
    _activeStep = 0xFFFF;
}

uint32_t ShowPlayer::position() const {
    if (!_playing) return 0;
    if (_paused) return _pauseMs - _startMs;
    return millis() - _startMs;
}

void ShowPlayer::triggerStep(uint16_t idx, uint8_t* channels) {
    memcpy(_from, channels, 512);
    memcpy(_to, channels, 512);

    for (uint16_t i = 0; i < _steps[idx].numCh; i++) {
        ChVal& cv = _chVals[_steps[idx].firstCh + i];
        if (cv.ch >= 1 && cv.ch <= 512)
            _to[cv.ch - 1] = cv.val;
    }

    _fadeDurMs   = _steps[idx].fade;
    _fadeStartMs = millis();
    _activeStep  = idx;
}

void ShowPlayer::tick(uint8_t* channels) {
    if (!_playing || _paused) return;

    uint32_t pos = millis() - _startMs;

    // Check if we've passed the end
    if (pos >= _duration) {
        if (_loop) {
            _startMs = millis();
            _activeStep = 0xFFFF;
            pos = 0;
        } else {
            // Apply final state and stop
            memcpy(channels, _to, 512);
            stop();
            return;
        }
    }

    // Find which step we should be on
    uint16_t targetStep = 0xFFFF;
    for (uint16_t i = 0; i < _stepCount; i++) {
        if (_steps[i].time <= pos) targetStep = i;
        else break;
    }

    // Trigger new step if needed
    if (targetStep != 0xFFFF && targetStep != _activeStep) {
        triggerStep(targetStep, channels);
    }

    // Interpolate fade
    if (_activeStep != 0xFFFF) {
        if (_fadeDurMs == 0) {
            memcpy(channels, _to, 512);
        } else {
            unsigned long elapsed = millis() - _fadeStartMs;
            if (elapsed >= _fadeDurMs) {
                memcpy(channels, _to, 512);
            } else {
                float t = (float)elapsed / (float)_fadeDurMs;
                for (int i = 0; i < 512; i++) {
                    channels[i] = _from[i] + (int16_t)(_to[i] - _from[i]) * t;
                }
            }
        }
    }
}
