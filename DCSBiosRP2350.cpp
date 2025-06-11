// #define PICO_BOARD
#define FOX1_BOARD

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "DcsBios.h"
#include "internal/FoxConfig.h"
#include "internal/Leds.h"
#include "i2c.pio.h"
#include "internal/pio_i2c.h"

// #include "pico/time.h"

bool heartbeatState = false;
uint32_t lastHeartbeat = 0;

void init_heartbeat_led() {
    gpio_init(HEARTBEAT_LED);
    gpio_set_dir(HEARTBEAT_LED, GPIO_OUT);
    gpio_put(HEARTBEAT_LED, 0); // Ensure off at boot
}

void update_heartbeat_led() {
    const uint32_t interval = 1000;  // ms

    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - lastHeartbeat >= interval) {
        heartbeatState = !heartbeatState;
        gpio_put(HEARTBEAT_LED, heartbeatState);
        lastHeartbeat = now;
    }
}

void init_i2c0() {
    i2c_init(i2c0, 400 * 1000);  // 400kHz
    gpio_set_function(I2C0_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C0_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C0_SDA);
    gpio_pull_up(I2C0_SCL);
}

void i2c0_scan_bus() {
    printf("\nI2C0 Bus Scan\n    ");
    for (int i = 0; i < 16; i++) {
        printf(" %X", i);
    }

    for (int addr = 0x03; addr <= 0x77; addr++) {
        if ((addr & 0x0F) == 0)
            printf("\n%02X: ", addr & 0xF0);

        // Skip reserved addresses
        if ((addr & 0x78) == 0 || (addr & 0x78) == 0x78) {
            printf(".  ");
            continue;
        }

        uint8_t dummy;
        int result = i2c_read_blocking(i2c0, addr, &dummy, 1, false);
        printf(result < 0 ? ".  " : "%02X ", addr);
    }

    printf("\nI2C0 scan complete.\n");
}


// // I2C I/O expander init
AW9523B aw_led(i2c0, AW_DEV_LED);  // Assuming `i2c0` is initialized
// AW9523B aw_io1(i2c0, AW_DEV_IO1);


// Ht16k33 ht16k33(i2c0, 0x70);  // I2C address for HT16K33

// // I2C control of LED
// DcsBios::LED floodLight(0x7420, 0x1000, &aw_led, 3, false, 128);

// // I2C read of switch
// DcsBios::Switch2Pos gearHandleSw("GEAR_HANDLE", &aw_io1, 4);

// WS2812 init
// WS2812 wsStrip(pio1, 0, NEO_DRIVE_PIN);  // WS2812 on PIO0, SM 0, NEO_DRIVE_PIN
// DcsBios::LED master_AA_LED(FA_18C_hornet_MASTER_MODE_AA_LT_AM, &wsStrip, 1, wsStrip.rgb(255, 0, 0), 100);
// DcsBios::LED master_AG_LED(FA_18C_hornet_MASTER_MODE_AG_LT_AM, &wsStrip, 2, wsStrip.rgb(255, 0, 0), 100);
// DcsBios::LED half_flaps_LED(FA_18C_hornet_FLP_LG_HALF_FLAPS_LT_AM, &wsStrip, 3, wsStrip.rgb(0, 0, 255), 100);
// DcsBios::LED full_flaps_LED(FA_18C_hornet_FLP_LG_FULL_FLAPS_LT_AM, &wsStrip, 4, wsStrip.rgb(0, 0, 255), 100);
// DcsBios::LED left_gear_LED(FA_18C_hornet_FLP_LG_LEFT_GEAR_LT_AM, &wsStrip, 5, wsStrip.rgb(0, 255, 0), 100);
// DcsBios::LED right_gear_LED(FA_18C_hornet_FLP_LG_RIGHT_GEAR_LT_AM, &wsStrip, 6, wsStrip.rgb(0, 255, 0), 100);
// DcsBios::LED nose_gear_LED(FA_18C_hornet_FLP_LG_NOSE_GEAR_LT_AM, &wsStrip, 7, wsStrip.rgb(0, 255, 0), 100);
// DcsBios::LED center_jett_LED(FA_18C_hornet_SJ_CTR_LT_AM, &wsStrip, 8, wsStrip.rgb(255, 255, 255), 100);
// DcsBios::LED ri_jett_LED(FA_18C_hornet_SJ_RI_LT_AM, &wsStrip, 9, wsStrip.rgb(255, 255, 255), 100);
// DcsBios::LED li_jett_LED(FA_18C_hornet_SJ_LI_LT_AM, &wsStrip, 10, wsStrip.rgb(255, 255, 255), 100);
// DcsBios::LED ro_jett_LED(FA_18C_hornet_SJ_RO_LT_AM, &wsStrip, 11, wsStrip.rgb(255, 255, 255), 100);
// DcsBios::LED lo_jett_LED(FA_18C_hornet_SJ_LO_LT_AM, &wsStrip, 12, wsStrip.rgb(255, 255, 255), 100);
// DcsBios::LED master_caution_LED(FA_18C_hornet_MASTER_CAUTION_LT_AM, &wsStrip, 13, wsStrip.rgb(255, 255, 0), 100);

// DcsBios::RotaryEncoderT<POLL_EVERY_TIME, DcsBios::TWO_STEPS_PER_DETENT> ufcComm1ChannelSelect("UFC_COMM1_CHANNEL_SELECT", "DEC", "INC", ENCODER2_B, ENCODER2_A);
// DcsBios::RotaryEncoderT<POLL_EVERY_TIME, DcsBios::TWO_STEPS_PER_DETENT> ufcComm2ChannelSelect("UFC_COMM2_CHANNEL_SELECT", "DEC", "INC", ENCODER1_B, ENCODER1_A);

DcsBios::Switch2Pos emerJettBtn("EMER_JETT_BTN", 6);

void pio_i2c_scan_bus(PIO pio, uint sm) {
    uart_puts(UART_ID, "\nLite I2C Bus Scan\n    ");
    for (int i = 0; i < 16; i++) {
        char hex[4];
        sprintf(hex, "%X ", i);
        uart_puts(UART_ID, hex);
    }

    for (int addr = 0x03; addr <= 0x77; addr++) {
        if ((addr & 0x0F) == 0)
            printf("\n%02X: ", addr & 0xF0);

        uint8_t dummy = 0;
        int result = pio_i2c_write_blocking(pio0, sm, addr, &dummy, 0);  // send addr only

        if (result == 0)
            printf("%02X ", addr);
        else
            printf(".  ");
    }
    uart_puts(UART_ID, "\nI2C scan complete.\n");
}

int main() {
    stdio_init_all();  // Initialize USB CDC
    init_heartbeat_led();
    sleep_ms(100);   // Wait for USB CDC to be ready

    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_puts(UART_ID, "RP2350 Started!\n");

    // Explicitly reference the function inside the DcsBios namespace
    multicore_launch_core1(DcsBios::core1_task);  
    uart_puts(UART_ID, "Core 1 task launched!\n");

    adc_init();

    init_i2c0();  // Initialize I2C0
    sleep_ms(100); 

    i2c0_scan_bus();  // Scan I2C0 bus
    aw_led.begin();  // <-- This is critical

    aw_led.enableLED(1);  // Enable LED 1
    for (int i = 0; i < 11; i++) {
        bool current = aw_led.readPin(1);
        aw_led.writePin(1, !current);
        sleep_ms(500);
    }
    printf("LED test complete\n");

    // if (!wsStrip.begin(24)) {  // You are using 6 pixels
    //     uart_puts(UART_ID, "WS2812 init failed!\n");
    // }
    // else {
    //     uart_puts(UART_ID, "WS2812 init success!\n");
    //     sleep_ms(100);
    // }


//     const uint32_t testColors[3] = {
//         wsStrip.rgb(255, 0, 0),  // Red
//         wsStrip.rgb(0, 255, 0),  // Green
//         wsStrip.rgb(0, 0, 255)   // Blue
//     };
    
//     for (int c = 0; c < 3; c++) {
//         wsStrip.clear();
//         for (int i = 0; i < 24; i++) {
//             wsStrip.setPixel(i, testColors[c]);
//         }
//         wsStrip.show();
//         sleep_ms(500);  // Hold each color
//     }
//     wsStrip.clear();
//     wsStrip.show();


//     PIO pio = pio0;
//     uint sm = 0;
//     uint offset = pio_add_program(pio, &i2c_program);
//     pio_i2c_init(pio, sm, offset, I2C0_SCL, I2C0_SDA);
//     pio_i2c_scan_bus(pio, sm);
//     printf("I2C bus scan complete!\n");


//     uint8_t rxbuf[1];
//     int result = pio_i2c_read_blocking(pio, sm, 0x70, rxbuf, 1);

//     if (result == 0) {
//         char msg[64];
//         printf(msg, "Read from 0x70: 0x%02X\n", rxbuf[0]);
//         printf(msg);
//     } else {
//         printf("Read from 0x70 failed\n");
//     }
//     printf("I2C read complete!\n");


//     // Set register pointer to 0x00
//     uint8_t reg = 0x00;
//     int wr = pio_i2c_write_blocking(pio, sm, 0x70, &reg, 1);

//     if (wr != 0) {
//         printf("Failed to write register pointer\n");
//     } else {
//         // Now read 1 byte
//         uint8_t rx = 0;
//         int rd = pio_i2c_read_blocking(pio, sm, 0x70, &rx, 1);

//         if (rd == 0) {
//             char out[64];
//             sprintf(out, "Read from 0x70 @ reg 0x00: 0x%02X\n", rx);
//             printf(out);
//         } else {
//             printf("Read from 0x70 failed\n");
//         }
//     }

//     // Turn on oscillator
//     uint8_t cmd1[] = {0x21};  // 0b00100001
//     pio_i2c_write_blocking(pio, sm, 0x70, cmd1, 1);

//     // Display on, no blink
//     uint8_t cmd2[] = {0x81};  // 0b10000001
//     pio_i2c_write_blocking(pio, sm, 0x70, cmd2, 1);

//     // Brightness max
//     uint8_t cmd3[] = {0xEF};  // 0xE0 | 15
//     pio_i2c_write_blocking(pio, sm, 0x70, cmd3, 1);

//     const uint8_t char_map[] = {
//         // Numbers 0-9
//         0b00111111, // 0
//         0b00000110, // 1
//         0b01011011, // 2
//         0b01001111, // 3
//         0b01100110, // 4
//         0b01101101, // 5
//         0b01111101, // 6
//         0b00000111, // 7
//         0b01111111, // 8
//         0b01101111, // 9
    
//         // Letters A-Z (only displayable ones)
//         0b01110111, // A
//         0b01111100, // b
//         0b00111001, // C
//         0b01011110, // d
//         0b01111001, // E
//         0b01110001, // F
//         0b00111101, // G
//         0b01110110, // H
//         0b00000110, // I
//         0b00011110, // J
//         0b00111000, // L
//         0b01010100, // n
//         0b01011100, // o
//         0b01110011, // P
//         0b01010000, // r
//         0b01101101, // S
//         0b01111000, // t
//         0b00111110, // U
//         0b00011100, // V
//         0b01101110  // Y
//     };

//     while (true) {
//     for (int i = 0; i < sizeof(char_map); i++) {
//         uint8_t buf[] = {
//             0x00,
//             char_map[i], 0x00,
//             char_map[i], 0x00,
//             char_map[i], 0x00,
//             char_map[i], 0x00
//         };
//         pio_i2c_write_blocking(pio0, 0, 0x70, buf, sizeof(buf));
//         sleep_ms(1000);
//     }
// }


    DcsBios::setup();  // Initialize DCS-BIOS framework
    printf("DCS-BIOS setup complete!\n");
    while (true) {
        printf("DCS-BIOS loop\n");
        DcsBios::loop(); // Handle input, output, and LED updates
        update_heartbeat_led();
        sleep_us(10);
    }
}
