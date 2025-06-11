#ifndef __HT16K33_H__
#define __HT16K33_H__

#include "hardware/i2c.h"
#include <stdint.h>
#include "hardware/pio.h"

class Ht16k33 {
    public:
        Ht16k33(PIO pio, uint sm, uint8_t address);
        void begin();
        void setBrightness(uint8_t level);
        void setBlinkRate(uint8_t rate);
        void setLED(uint8_t pos, bool on);
        void writeDisplay();
        void clear();
        void printNumber(int num);
        void printHex(uint16_t value);
        void setRawDigit(uint8_t position, uint8_t segmentMask);
    
    private:
        PIO pio_;
        uint sm_;
        uint8_t address_;
        uint8_t displayBuffer_[16];
    };

#endif // __HT16K33_H__
