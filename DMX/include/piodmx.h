#ifndef _pio_dmx_h_
#define _pio_dmx_h_

#include "DmxOutput.h"
#include "DmxOutput.pio.h"
#include "DmxInput.h"
#include "DmxInput.pio.h"

class DMX {
    public:
    DMX(PIO pio = pio0);
    ~DMX();
    void begin(int pinp, int pinn, int pin_out_en, int pin_in_en);
    void sendDMX();
    void setChannel(int channel, int value);
    void writeBuffer(uint8_t *buffer, bool noStartCode = true);
    void unasfeSetChannel(int channel, int value);
    void unsafeWriteBuffer(uint8_t *buffer, bool noStartCode = true);
    bool busy();
    uint getprgm_offseto() {return dmxo->getprgm_offset();};
    DmxOutput::return_code _ostatus;
    DmxInput::return_code _istatus;
    void setOutput(bool);
    void setInput(bool);
    uint8_t *readBuffer() {return idmxData;};



    private:
    uint8_t odmxData[513];
    uint8_t idmxData[513];
    uint universeSize = 513;
    uint pinp;
    uint pinn;
    uint pin_out_en;
    uint pin_in_en;
    PIO _pio;
    bool data_update = false;


    protected:
    DmxOutput *dmxo;
    DmxInput *dmxi;
};

#endif // _pio_dmx_h_
