// #define PICO_BOARD
#define FOX2_BOARD

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "DcsBios.h"
#include "internal/FoxConfig.h"
#include "internal/Leds.h"
#include "i2c.pio.h"
#include "internal/pio_i2c.h"
#include "internal/heartbeat.h"
#include "internal/ht16k33a.h"
#include "internal/SegmentFont.h"
#include "internal/DeviceAddress.h"
#include "internal/BoardMode.h"
#include "internal/rs485.h"
#include <cstring>
#include <stdlib.h>  // for rand()
#include <time.h>    // for seeding rand()

#define SEG_r  0b01010000
#define SEG_G  0b00111101
#define SEG_b  0b01111100


Ht16k33* commDisplay = nullptr;
uart_inst_t* rs485_uart = uart0; // Control UART in main

int lastEncState = 0;
int encoderDelta = 0;
int ledCount = 1;  // Current number of LEDs to light
int red = 50, green = 0, blue = 0;
int currentChannel = 0;  // 0 = R, 1 = G, 2 = B
int lastRgbEncState = 0;
bool lastButtonState = true; // Active low

bool rainbowMode = false;
int rainbowDelay = 200; // ms, adjustable with encoder
absolute_time_t lastRainbowTime;

// Encoder 3 state
int lastRainbowEncState = 0;
bool lastRainbowButtonState = true;
int globalIntensity = 20;  // Default brightness for rainbow mode


// // WS2812 init
WS2812 wsStrip(pio1, 0, NEO_DRIVE_PIN);  // WS2812 on PIO0, SM 0, NEO_DRIVE_PIN

DcsBios::Switch2Pos emerJettBtn("EMER_JETT_BTN", 6);

void updateCommDisplay();  // Forward declaration
void updateLedStrip();  // Forward declaration
void updateRgbDisplay();  // Forward declaration

extern const uint8_t digitToSegment[];  // From SegmentFont.h



void updateCommDisplay(const char* color , uint8_t value) {
    if (!commDisplay) return;

    commDisplay->clear();

    uint8_t hundreds = (value / 100) % 10;
    uint8_t tens     = (value / 10) % 10;
    uint8_t ones     = value % 10;

    if (strcmp(color, "r") == 0) {
        commDisplay->setRawDigit(0, SEG_r); // 'r' for red
    } else if (strcmp(color, "G") == 0) {
        commDisplay->setRawDigit(0, SEG_G); // 'G' for green
    } else if (strcmp(color, "b") == 0) {
        commDisplay->setRawDigit(0, SEG_b); // 'b' for blue
    } else {
        commDisplay->setRawDigit(0, 0); // Blank or fallback
    }

    commDisplay->setRawDigit(1, digitToSegment[hundreds]);
    commDisplay->setRawDigit(2, digitToSegment[tens]);
    commDisplay->setRawDigit(3, digitToSegment[ones]);
    commDisplay->writeDisplay();
}




void pollEncoderLEDCount() {
    static int transitionCount = 0;
    static int lastState = 0;
    int state = (gpio_get(ENCODER2_B) << 1) | gpio_get(ENCODER2_A);

    if (state != lastState) {
        // Decode quadrature
        switch (lastState) {
            case 0: if (state == 1) transitionCount++; if (state == 2) transitionCount--; break;
            case 1: if (state == 3) transitionCount++; if (state == 0) transitionCount--; break;
            case 3: if (state == 2) transitionCount++; if (state == 1) transitionCount--; break;
            case 2: if (state == 0) transitionCount++; if (state == 3) transitionCount--; break;
        }
        lastState = state;

        if (transitionCount >= 2) {
            if (ledCount < 24) ledCount++;
            printf("Encoder + -> LEDs: %d\n", ledCount);
            updateLedStrip();
            transitionCount = 0;
        } else if (transitionCount <= -2) {
            if (ledCount > 0) ledCount--;
            printf("Encoder - -> LEDs: %d\n", ledCount);
            updateLedStrip();
            transitionCount = 0;
        }
    }
}

void pollEncoderRGB() {
    static int transitionCount = 0;
    int state = (gpio_get(ENCODER1_B) << 1) | gpio_get(ENCODER1_A);

    if (state != lastRgbEncState) {
        switch (lastRgbEncState) {
            case 0: if (state == 1) transitionCount++; if (state == 2) transitionCount--; break;
            case 1: if (state == 3) transitionCount++; if (state == 0) transitionCount--; break;
            case 3: if (state == 2) transitionCount++; if (state == 1) transitionCount--; break;
            case 2: if (state == 0) transitionCount++; if (state == 3) transitionCount--; break;
        }
        lastRgbEncState = state;

        if (transitionCount >= 2 || transitionCount <= -2) {
            int delta = (transitionCount >= 2) ? 1 : -1;
            transitionCount = 0;

            if (rainbowMode) {
                globalIntensity = std::clamp(globalIntensity + delta, 0, 255);
                printf("Rainbow Intensity: %d\n", globalIntensity);
            } else {
                if (currentChannel == 0) red = std::clamp(red + delta, 0, 255);
                else if (currentChannel == 1) green = std::clamp(green + delta, 0, 255);
                else blue = std::clamp(blue + delta, 0, 255);

                updateRgbDisplay();
                updateLedStrip();
            }
        }
    }
}


void pollEncoderRgbButton() {
    bool current = gpio_get(ENCODER1_SW);  // Active LOW
    if (lastButtonState == true && current == false) {
        currentChannel = (currentChannel + 1) % 3;
        printf("Switched channel: %d\n", currentChannel);
        updateRgbDisplay();
    }
    lastButtonState = current;
}


void updateLedStrip() {
    for (int i = 0; i < 24; ++i) {
        if (i < ledCount) {
            if (rainbowMode) {
                // Random color with brightness limited by globalIntensity
                uint8_t r = rand() % (globalIntensity + 1);
                uint8_t g = rand() % (globalIntensity + 1);
                uint8_t b = rand() % (globalIntensity + 1);
                wsStrip.setPixel(i, wsStrip.rgb(r, g, b));
            } else {
                // Static RGB values in normal mode
                wsStrip.setPixel(i, wsStrip.rgb(red, green, blue));
            }
        } else {
            wsStrip.setPixel(i, 0); // Off
        }
    }
    wsStrip.show();
}




void updateRainbowColors() {
    for (int i = 0; i < 24; ++i) {
        if (i < ledCount) {
            uint8_t r = rand() % (globalIntensity + 1);
            uint8_t g = rand() % (globalIntensity + 1);
            uint8_t b = rand() % (globalIntensity + 1);
            wsStrip.setPixel(i, wsStrip.rgb(r, g, b));
        } else {
            wsStrip.setPixel(i, 0);
        }
    }
    if (commDisplay) {
        commDisplay->clear();
        for (int i = 0; i < 4; ++i) {
            uint8_t randIndex = rand() % (sizeof(digitToSegment) / sizeof(digitToSegment[0]));
            commDisplay->setRawDigit(i, digitToSegment[randIndex]);
        }
        commDisplay->writeDisplay();
    }
    wsStrip.show();
}

void pollEncoderRainbowSpeed() {
    if (!rainbowMode) return;

    static int transitionCount = 0;
    int state = (gpio_get(ENCODER3_B) << 1) | gpio_get(ENCODER3_A);

    if (state != lastRainbowEncState) {
        switch (lastRainbowEncState) {
            case 0: if (state == 1) transitionCount++; if (state == 2) transitionCount--; break;
            case 1: if (state == 3) transitionCount++; if (state == 0) transitionCount--; break;
            case 3: if (state == 2) transitionCount++; if (state == 1) transitionCount--; break;
            case 2: if (state == 0) transitionCount++; if (state == 3) transitionCount--; break;
        }
        lastRainbowEncState = state;

        if (transitionCount >= 2 || transitionCount <= -2) {
            int delta = (transitionCount >= 2) ? -10 : 10;
            rainbowDelay = std::clamp(rainbowDelay + delta, 1, 1000);
            printf("Rainbow delay: %dms\n", rainbowDelay);
            transitionCount = 0;
        }
    }
}

void pollEncoderRainbowButton() {
    bool current = gpio_get(ENCODER3_SW);  // Active LOW
    if (lastRainbowButtonState == true && current == false) {
        rainbowMode = !rainbowMode;
        printf("Rainbow mode: %s\n", rainbowMode ? "ON" : "OFF");
        if (!rainbowMode) {
            updateRgbDisplay();   
            updateLedStrip();
        }
    }
    lastRainbowButtonState = current;
}

void updateRgbDisplay() {
    switch (currentChannel) {
        case 0: updateCommDisplay("r", red); break;
        case 1: updateCommDisplay("G", green); break;
        case 2: updateCommDisplay("b", blue); break;
    }
}

int main() {
    stdio_init_all();  // Initialize USB CDC
    DcsBios::initHeartbeat(HEARTBEAT_LED);  // Initialize heartbeat LED

    gpio_init(ENCODER1_A); gpio_set_dir(ENCODER1_A, GPIO_IN); gpio_pull_up(ENCODER1_A);
    gpio_init(ENCODER1_B); gpio_set_dir(ENCODER1_B, GPIO_IN); gpio_pull_up(ENCODER1_B);
    gpio_init(ENCODER1_SW); gpio_set_dir(ENCODER1_SW, GPIO_IN); gpio_pull_up(ENCODER1_SW);
    
    gpio_init(ENCODER2_A); gpio_set_dir(ENCODER2_A, GPIO_IN); gpio_pull_up(ENCODER2_A);
    gpio_init(ENCODER2_B); gpio_set_dir(ENCODER2_B, GPIO_IN); gpio_pull_up(ENCODER2_B);
    gpio_init(ENCODER2_SW); gpio_set_dir(ENCODER2_SW, GPIO_IN); gpio_pull_up(ENCODER2_SW);

    gpio_init(ENCODER3_A); gpio_set_dir(ENCODER3_A, GPIO_IN); gpio_pull_up(ENCODER3_A);
    gpio_init(ENCODER3_B); gpio_set_dir(ENCODER3_B, GPIO_IN); gpio_pull_up(ENCODER3_B);
    gpio_init(ENCODER3_SW); gpio_set_dir(ENCODER3_SW, GPIO_IN); gpio_pull_up(ENCODER3_SW);
    lastRainbowEncState = (gpio_get(ENCODER3_B) << 1) | gpio_get(ENCODER3_A);
    srand(time_us_64());  // Seed rand once

    lastRgbEncState = (gpio_get(ENCODER1_B) << 1) | gpio_get(ENCODER1_A);
    
    
    sleep_ms(100);   // Wait for USB CDC to be ready


    
    // Explicitly reference the function inside the DcsBios namespace
    multicore_launch_core1(DcsBios::core1_task);  
    printf("Core 1 task launched!\n");


    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &i2c_program);
    pio_i2c_init(pio, sm, offset, I2C0_SCL, I2C0_SDA);

    commDisplay = new Ht16k33(pio, sm, 0x70);
    commDisplay->begin();

    // Display "0000" at boot
    commDisplay->setRawDigit(0, digitToSegment[6]);
    commDisplay->setRawDigit(1, digitToSegment[9]);
    commDisplay->setRawDigit(2, digitToSegment[6]);
    commDisplay->setRawDigit(3, digitToSegment[9]);
    commDisplay->writeDisplay();

    updateRgbDisplay();
  

    if (!wsStrip.begin(24)) {  // You are using 6 pixels
    }
    else {
        printf("WS2812 init succeeded. Turning LEDs blue for 3 seconds...\n");
        // Turn all 24 LEDs red
        for (int i = 0; i < 24; ++i) {
            wsStrip.setPixel(i, wsStrip.rgb(50, 0, 0));  // Pure blue
        }
        wsStrip.show();
        
        sleep_ms(333);  // Wait 3 seconds
        // Turn all 24 LEDs blue
        for (int i = 0; i < 24; ++i) {
            wsStrip.setPixel(i, wsStrip.rgb(0, 50, 0));  // Pure blue
        }
        wsStrip.show();
        sleep_ms(333);  // Wait 3 seconds

        for (int i = 0; i < 24; ++i) {
            wsStrip.setPixel(i, wsStrip.rgb(0, 0, 50));  // Pure blue
        }
        wsStrip.show();
        
        sleep_ms(333);  // Wait 3 seconds

        // Turn all LEDs off
        for (int i = 0; i < 24; ++i) {
            wsStrip.setPixel(i, 0);  // Off
        }
        wsStrip.show();
        // updateLedStrip();
    }

    updateLedStrip();

    DcsBios::setup();  // Initialize DCS-BIOS framework
    while (true) {
        pollEncoderLEDCount();
        pollEncoderRGB();
        pollEncoderRgbButton();
        pollEncoderRainbowSpeed();
        pollEncoderRainbowButton();

        if (rainbowMode && absolute_time_diff_us(lastRainbowTime, get_absolute_time()) >= rainbowDelay * 1000) {
            updateRainbowColors();
            lastRainbowTime = get_absolute_time();
        }

        DcsBios::loop();
        DcsBios::updateHeartbeat();
        sleep_us(100);
    }
}