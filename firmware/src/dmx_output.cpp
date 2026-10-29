#include "dmx_output.h"
#include "config.h"
#include "driver/uart.h"

void DmxOutput::init() {
    uart_config_t cfg = {};
    cfg.baud_rate    = 250000;
    cfg.data_bits    = UART_DATA_8_BITS;
    cfg.parity       = UART_PARITY_DISABLE;
    cfg.stop_bits    = UART_STOP_BITS_2;
    cfg.flow_ctrl    = UART_HW_FLOWCTRL_DISABLE;
    cfg.source_clk   = UART_SCLK_APB;

    uart_param_config((uart_port_t)DMX_UART_NUM, &cfg);
    uart_set_pin((uart_port_t)DMX_UART_NUM, DMX_TX_PIN, DMX_RX_PIN, -1, -1);
    uart_driver_install((uart_port_t)DMX_UART_NUM, 1024, 0, 0, NULL, 0);

    pinMode(DMX_DE_PIN, OUTPUT);
    digitalWrite(DMX_DE_PIN, HIGH);
}

void DmxOutput::sendFrame() {
    // Break: invert TX line for DMX_BREAK_US
    uart_set_line_inverse((uart_port_t)DMX_UART_NUM, UART_SIGNAL_TXD_INV);
    delayMicroseconds(DMX_BREAK_US);
    uart_set_line_inverse((uart_port_t)DMX_UART_NUM, 0);
    delayMicroseconds(DMX_MAB_US);

    uint8_t frame[513];
    frame[0] = 0x00; // DMX start code

    if (_master == 255) {
        memcpy(frame + 1, _ch, DMX_CHANNELS);
    } else {
        for (int i = 0; i < DMX_CHANNELS; i++)
            frame[i + 1] = (uint8_t)((uint16_t)_ch[i] * _master / 255);
    }

    uart_write_bytes((uart_port_t)DMX_UART_NUM, frame, 513);
    uart_wait_tx_done((uart_port_t)DMX_UART_NUM, pdMS_TO_TICKS(50));
}

void DmxOutput::setChannel(uint16_t ch, uint8_t value) {
    if (ch >= 1 && ch <= 512) _ch[ch - 1] = value;
}

uint8_t DmxOutput::getChannel(uint16_t ch) const {
    return (ch >= 1 && ch <= 512) ? _ch[ch - 1] : 0;
}

void DmxOutput::setChannels(const uint8_t* data, uint16_t count) {
    if (count > 512) count = 512;
    memcpy(_ch, data, count);
}

void DmxOutput::blackout() { memset(_ch, 0, 512); }
void DmxOutput::setMaster(uint8_t v) { _master = v; }
uint8_t DmxOutput::getMaster() const { return _master; }
const uint8_t* DmxOutput::getRaw() const { return _ch; }
