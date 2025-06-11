#ifndef __SEGMENT_FONT_H__
#define __SEGMENT_FONT_H__

#include <stdint.h>

#define SEG_C  0b00111001
#define SEG_M  0b01010100  
#define SEG_G  0b00111101
#define SEG_S  0b01101101


// Common segment map for 7-segment display
const uint8_t digitToSegment[] = {
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111, // 9

    // Letters A-Z (only displayable subset)
    0b01110111, // A
    0b01111100, // b
    0b00111001, // C
    0b01011110, // d
    0b01111001, // E
    0b01110001, // F
    0b00111101, // G
    0b01110110, // H
    0b00000110, // I
    0b00011110, // J
    0b00111000, // L
    0b01010100, // n
    0b01011100, // o
    0b01110011, // P
    0b01010000, // r
    0b01101101, // S
    0b01111000, // t
    0b00111110, // U
    0b00011100, // V
    0b01101110  // Y
};

#endif // __SEGMENT_FONT_H__
