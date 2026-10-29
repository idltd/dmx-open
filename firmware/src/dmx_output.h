#pragma once
#include <Arduino.h>

class DmxOutput {
public:
    void init();
    void sendFrame();
    void setChannel(uint16_t ch, uint8_t value);
    uint8_t getChannel(uint16_t ch) const;
    void setChannels(const uint8_t* data, uint16_t count);
    void blackout();
    void setMaster(uint8_t value);
    uint8_t getMaster() const;
    const uint8_t* getRaw() const;

private:
    uint8_t _ch[512] = {0};
    uint8_t _master = 255;
};
