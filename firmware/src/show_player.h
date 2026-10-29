#pragma once
#include <Arduino.h>

struct ChVal {
    uint16_t ch;  // 1-512
    uint8_t  val; // 0-255
};

struct ShowStep {
    uint32_t time;     // ms from show start
    uint16_t fade;     // crossfade duration in ms
    uint16_t firstCh;  // index into ChVal array
    uint16_t numCh;    // count of channel values in this step
};

class ShowPlayer {
public:
    ~ShowPlayer() { unload(); }

    bool load(const char* name);
    void unload();
    void play();
    void pause();
    void stop();
    void tick(uint8_t* channels);

    bool isPlaying() const  { return _playing && !_paused; }
    bool isPaused() const   { return _playing && _paused; }
    bool isLoaded() const   { return _loaded; }
    bool loops() const      { return _loop; }
    const char* name() const { return _name; }
    uint32_t position() const;
    uint32_t duration() const { return _duration; }

private:
    ShowStep* _steps   = nullptr;
    ChVal*    _chVals  = nullptr;
    uint16_t  _stepCount  = 0;
    uint16_t  _chValCount = 0;
    char      _name[64]   = {0};
    uint32_t  _duration   = 0;
    bool      _loop    = false;
    bool      _loaded  = false;
    bool      _playing = false;
    bool      _paused  = false;
    unsigned long _startMs = 0;
    unsigned long _pauseMs = 0;
    uint16_t  _activeStep = 0xFFFF;

    uint8_t  _from[512] = {0};
    uint8_t  _to[512]   = {0};
    unsigned long _fadeStartMs = 0;
    uint16_t _fadeDurMs = 0;

    void triggerStep(uint16_t idx, uint8_t* channels);
};
