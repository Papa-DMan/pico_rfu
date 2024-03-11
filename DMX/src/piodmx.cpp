#include "piodmx.h"
#include "DmxOutput.pio.h"

DMX::DMX(PIO pio) {
    _pio = pio;
    dmxp = new DmxOutput();
    //dmxn = new DmxOutput();
    for (int i = 0; i < 513; i++) {
        dmxData[i] = i % 256;
        //ndmxData[i] = (255 - i) % 256;
    }
}

DMX::~DMX() {
    dmxp->end();
    //dmxn->end();
    delete dmxp;
   // delete dmxn;
}

void DMX::begin(int pinp/*, int pinn*/) {
    this->pinp = pinp;
    //this->pinn = pinn;
    uint prgm_offset = pio_add_program(_pio, &DmxOutput_program);
    _pstatus = dmxp->begin(pinp, prgm_offset, _pio, false);
    //_nstatus = dmxn->begin(pinn, prgm_offset, _pio, true);
    
}

void DMX::sendDMX() {
    dmxp->write(dmxData, universeSize);
}

bool DMX::busy() {
    return dmxp->busy() || /*dmxn->busy() ||*/ data_update;
}

void DMX::setChannel(int channel, int value) {
    while (dmxp->busy() /*|| dmxn->busy()*/) {
        
    }
    data_update = true;
    if (channel < 1 || channel > 512)
        return;
    dmxData[channel] = value;
    //ndmxData[channel] = 255 - value;
    data_update = false;
}

void DMX::unasfeSetChannel(int channel, int value) {
    if (channel < 1 || channel > 512)
        return;
    dmxData[channel] = value;
    //ndmxData[channel] = 255 - value;
}

void DMX::writeBuffer(uint8_t *buffer, bool noStartCode) {
    while (dmxp->busy() /*|| dmxn->busy()*/) {

    }
    data_update = true;
    for (int i = noStartCode; i < 513; i++) {
        dmxData[i] = buffer[i];
        //ndmxData[i] = 255 - buffer[i];
    }
    data_update = false;
}

void DMX::unsafeWriteBuffer(uint8_t *buffer, bool noStartCode) {
    for (int i = noStartCode; i < 513; i++) {
        dmxData[i] = buffer[i];
        //ndmxData[i] = 255 - buffer[i];
    }
}