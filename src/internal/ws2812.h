#ifndef __WS2812_H
#define __WS2812_H

#include "hardware/pio.h"
#include <cstdint>


class WS2812 {
    public:
        WS2812(PIO pio, uint sm, uint pin, bool isRgbw = false);
        ~WS2812();
    
        bool begin(uint16_t numPixels);
        void setPixel(uint16_t index, uint32_t grb);
        void show();
        void clear();
        void setBrightness(uint8_t brightness);
    
        uint32_t rgb(uint8_t r, uint8_t g, uint8_t b);
        uint32_t rgbw(uint8_t r, uint8_t g, uint8_t b, uint8_t w);
    
    private:
        PIO _pio;
        uint _sm;
        uint _pin;
        bool _isRgbw;
        uint _offset;
    
        uint16_t _numPixels;
        uint32_t* _pixelBuffer;
        uint8_t _brightness;
    };
    
#endif
