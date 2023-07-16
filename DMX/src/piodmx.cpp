#include "piodmx.h"
#include "DmxOutput.pio.h"

DMX::DMX(PIO pio) {
    _pio = pio;
    dmxo = new DmxOutput();
    dmxi = new DmxInput();
    //dmxn = new DmxOutput();
    for (int i = 0; i < 513; i++) {
        odmxData[i] = i % 256;
    }
}

DMX::~DMX() {
    dmxo->end();
    dmxi->end();
    delete dmxo;
    delete dmxi;
}

void DMX::begin(int pino, int pini, int out_en, int in_en) {
    this->pinp = pinp;
    this->pinn = pinn;
    uint prgm_offset = pio_add_program(_pio, &DmxOutput_program);
    _ostatus = dmxo->begin(pino, prgm_offset, _pio, false);
    _istatus = dmxi->begin(pini, 1, 512, _pio);
    dmxi->read_async(idmxData);
    this->pin_in_en = in_en;
    this->pin_out_en = out_en;
}

void DMX::sendDMX() {
    dmxo->write(odmxData, universeSize);
    //enable both sm 0 and 1 on pio0 in sync (32 bit word bits 3:0) so for example 0b00000000000000000000000000001111
    pio_enable_sm_mask_in_sync(pio0, 0b00000000000000000000000000000001);
}

bool DMX::busy() {
    return dmxo->busy() || data_update;
}

void DMX::setChannel(int channel, int value) {
    while (dmxo->busy()) {
        
    }
    data_update = true;
    if (channel < 1 || channel > 512)
        return;
    odmxData[channel] = value;
    data_update = false;
}

void DMX::unasfeSetChannel(int channel, int value) {
    if (channel < 1 || channel > 512)
        return;
    odmxData[channel] = value;
}

void DMX::writeBuffer(uint8_t *buffer, bool noStartCode) {
    while (dmxo->busy()) {

    }
    data_update = true;
    for (int i = noStartCode; i < 513; i++) {
        odmxData[i] = buffer[i];
    }
    data_update = false;
}

void DMX::unsafeWriteBuffer(uint8_t *buffer, bool noStartCode) {
    for (int i = noStartCode; i < 513; i++) {
        odmxData[i] = buffer[i];
    }
}

void DMX::setOutput(bool enable) {
    gpio_put(pin_out_en, enable);
}

void DMX::setInput(bool enable) {
    gpio_put(pin_in_en, !enable);
}