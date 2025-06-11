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

Ht16k33* commDisplay = nullptr;
uart_inst_t* rs485_uart = uart0; // Control UART in main


// // WS2812 init
WS2812 wsStrip(pio1, 0, NEO_DRIVE_PIN);  // WS2812 on PIO0, SM 0, NEO_DRIVE_PIN
DcsBios::LED master_AA_LED(FA_18C_hornet_MASTER_MODE_AA_LT_AM, &wsStrip, NEO_LED1, wsStrip.rgb(255, 0, 0), 100);
DcsBios::LED master_AG_LED(FA_18C_hornet_MASTER_MODE_AG_LT_AM, &wsStrip, NEO_LED9, wsStrip.rgb(255, 0, 0), 100);
DcsBios::LED master_caution_LED(FA_18C_hornet_MASTER_CAUTION_LT_AM, &wsStrip, NEO_LED17, wsStrip.rgb(255, 255, 0), 100);

DcsBios::LED nose_gear_LED(FA_18C_hornet_FLP_LG_NOSE_GEAR_LT_AM, &wsStrip, NEO_LED7, wsStrip.rgb(0, 255, 0), 100);
DcsBios::LED left_gear_LED(FA_18C_hornet_FLP_LG_LEFT_GEAR_LT_AM, &wsStrip, NEO_LED14, wsStrip.rgb(0, 255, 0), 100);
DcsBios::LED right_gear_LED(FA_18C_hornet_FLP_LG_RIGHT_GEAR_LT_AM, &wsStrip, NEO_LED16, wsStrip.rgb(0, 255, 0), 100);
DcsBios::LED landingGearHandleLt(FA_18C_hornet_LANDING_GEAR_HANDLE_LT_AM, &wsStrip, NEO_LED23, wsStrip.rgb(0, 255, 0), 100);

DcsBios::LED center_jett_LED(FA_18C_hornet_SJ_CTR_LT_AM, &wsStrip, NEO_LED3, wsStrip.rgb(255, 255, 255), 100);
DcsBios::LED ri_jett_LED(FA_18C_hornet_SJ_RI_LT_AM, &wsStrip, NEO_LED11, wsStrip.rgb(255, 255, 255), 100);
DcsBios::LED li_jett_LED(FA_18C_hornet_SJ_LI_LT_AM, &wsStrip, NEO_LED12, wsStrip.rgb(255, 255, 255), 100);
DcsBios::LED ro_jett_LED(FA_18C_hornet_SJ_RO_LT_AM, &wsStrip, NEO_LED19, wsStrip.rgb(255, 255, 255), 100);
DcsBios::LED lo_jett_LED(FA_18C_hornet_SJ_LO_LT_AM, &wsStrip, NEO_LED20, wsStrip.rgb(255, 255, 255), 100);

DcsBios::RotaryEncoderT<POLL_EVERY_TIME, DcsBios::TWO_STEPS_PER_DETENT> ufcComm1ChannelSelect("UFC_COMM1_CHANNEL_SELECT", "DEC", "INC", ENCODER2_B, ENCODER2_A);
DcsBios::RotaryEncoderT<POLL_EVERY_TIME, DcsBios::TWO_STEPS_PER_DETENT> ufcComm2ChannelSelect("UFC_COMM2_CHANNEL_SELECT", "DEC", "INC", ENCODER1_B, ENCODER1_A);
DcsBios::RotaryEncoderT<POLL_EVERY_TIME, DcsBios::TWO_STEPS_PER_DETENT> hudSymBrt("HUD_SYM_BRT", "-3200", "+3200", ENCODER3_B, ENCODER3_A);

DcsBios::Switch2Pos ufcComm1Pull("UFC_COMM1_PULL", ENCODER2_SW);
DcsBios::Switch2Pos ufcComm2Pull("UFC_COMM2_PULL", ENCODER1_SW);
DcsBios::ActionButton hudSymBrtSelectToggle("HUD_SYM_BRT_SELECT", "TOGGLE", ENCODER3_SW);

DcsBios::Switch2Pos emerJettBtn("EMER_JETT_BTN", 6);

void updateCommDisplay();  // Forward declaration

void onComm1ChannelNumericChange(unsigned int newValue) {
    updateCommDisplay();
}
DcsBios::IntegerBuffer comm1ChannelNumericBuffer(FA_18C_hornet_COMM1_CHANNEL_NUMERIC, onComm1ChannelNumericChange);

void onComm2ChannelNumericChange(unsigned int newValue) {
    updateCommDisplay();
}
DcsBios::IntegerBuffer comm2ChannelNumericBuffer(FA_18C_hornet_COMM2_CHANNEL_NUMERIC, onComm2ChannelNumericChange);

extern const uint8_t digitToSegment[];  // From SegmentFont.h



void updateCommDisplay() {
    if (!commDisplay) return;

    int comm1 = comm1ChannelNumericBuffer.getData();
    int comm2 = comm2ChannelNumericBuffer.getData();

    commDisplay->clear();

    // Handle COMM1 (first 2 digits)
    if (comm1 < 100) {
        if (comm1 == 23) commDisplay->setRawDigit(0, SEG_C);
        else if (comm1 == 22) commDisplay->setRawDigit(0, SEG_M);
        else if (comm1 == 21) commDisplay->setRawDigit(0, SEG_G);
        else if (comm1 == 24) commDisplay->setRawDigit(0, SEG_S);
        else {
            if (comm1 >= 10)
                commDisplay->setRawDigit(0, digitToSegment[comm1 / 10]);
            commDisplay->setRawDigit(1, digitToSegment[comm1 % 10] | 0b10000000);  // add decimal
        }
    }

    // Handle COMM2 (last 2 digits)
    if (comm2 < 100) {
        if (comm2 == 23) commDisplay->setRawDigit(2, SEG_C);
        else if (comm2 == 22) commDisplay->setRawDigit(2, SEG_M);
        else if (comm2 == 21) commDisplay->setRawDigit(2, SEG_G);
        else if (comm2 == 24) commDisplay->setRawDigit(2, SEG_S);
        else {
            if (comm2 >= 10)
                commDisplay->setRawDigit(2, digitToSegment[comm2 / 10]);
            commDisplay->setRawDigit(3, digitToSegment[comm2 % 10]);
        }
    }

    commDisplay->writeDisplay();
}



int main() {
    stdio_init_all();  // Initialize USB CDC
    DcsBios::initHeartbeat(HEARTBEAT_LED);  // Initialize heartbeat LED
    sleep_ms(5000);   // Wait for USB CDC to be ready

    DcsBios::initDeviceAddressPins(ADDR0, ADDR1, ADDR2, ADDR3);
    uint8_t boardAddress = DcsBios::readDeviceAddress();

    DcsBios::BoardMode board = DcsBios::determineBoardMode(boardAddress);
    printf("Board address: 0x%X\n", boardAddress);

    switch (board.mode) {
        case DcsBios::BoardModeType::HOST:
            printf("HOST MODE\n");
            break;
        case DcsBios::BoardModeType::SLAVE:
            printf("SLAVE MODE\n");
            break;
        case DcsBios::BoardModeType::USB_ONLY:
            printf("STANDALONE USB MODE\n");
            break;
        case DcsBios::BoardModeType::RS485_TERMINAL:
            printf("RS485 TERMINAL MODE\n");
            break;    
        default:
            printf("INVALID ADDRESS\n");
            break;
    }
    DcsBios::currentBoardMode = board;
    DcsBios::init_rs485_uart(rs485_uart, UART0_TX, UART0_RX, RS485_EN, 250000);
    
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

    if (!wsStrip.begin(24)) {  // You are using 6 pixels
    }
    else {
        sleep_ms(100);
    }

    DcsBios::setup();  // Initialize DCS-BIOS framework
    while (true) {
        DcsBios::loop(); // Handle input, output, and LED updates
        DcsBios::updateHeartbeat(); // Update heartbeat LED
        sleep_us(10);
    }
}