#ifndef _pio_dmx_h_
#define _pio_dmx_h_

#include "DmxOutput.h"
#include "DmxOutput.pio.h"

class DMX {
    public:
    DMX(PIO pio = pio0);
    ~DMX();
    void begin(int pinp/*, int pinn*/);
    void sendDMX();
    void setChannel(int channel, int value);
    void writeBuffer(uint8_t *buffer, bool noStartCode = true);
    void unasfeSetChannel(int channel, int value);
    void unsafeWriteBuffer(uint8_t *buffer, bool noStartCode = true);
    bool busy();
    void forceBusy(bool busy) {this->data_update = busy;};
    uint getprgm_offsetp() {return dmxp->getprgm_offset();};
    void getshadowbuff(uint8_t *buffer) {for (int i = 1; i < 513; i++) buffer[i] = dmxData[i];};
    //uint getprgm_offsetn() {return dmxn->getprgm_offset();};
    DmxOutput::return_code _pstatus;
    //DmxOutput::return_code _nstatus;


    private:
    uint8_t dmxData[513];
    //uint8_t ndmxData[513];
    uint universeSize = 513;
    uint pinp;
    //uint pinn;
    PIO _pio;
    bool data_update = false;


    protected:
    DmxOutput *dmxp;
    //DmxOutput *dmxn;
};

#endif // _pio_dmx_h_