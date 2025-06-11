#ifndef __AW9523B_H
#define __AW9523B_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"

// I2C default address for AW9523B (configurable via ADDR pin)
#define AW9523B_ADDRESS 0x58

// Register map
#define AW9523B_REG_INPUT_PORT0       0x00
#define AW9523B_REG_INPUT_PORT1       0x01
#define AW9523B_REG_OUTPUT_PORT0      0x02
#define AW9523B_REG_OUTPUT_PORT1      0x03
#define AW9523B_REG_CONFIG_PORT0      0x04
#define AW9523B_REG_CONFIG_PORT1      0x05
#define AW9523B_REG_INT_PORT0         0x06
#define AW9523B_REG_INT_PORT1         0x07
#define AW9523B_REG_ID                0x10
#define AW9523B_REG_CTL               0x11
#define AW9523B_REG_LED_MODE_PORT0    0x12
#define AW9523B_REG_LED_MODE_PORT1    0x13
#define AW9523B_REG_DIM0              0x24
#define AW9523B_REG_DIM1              0x25
#define AW9523B_REG_DIM2              0x26
#define AW9523B_REG_DIM3              0x27
#define AW9523B_REG_DIM4              0x28
#define AW9523B_REG_DIM5              0x29
#define AW9523B_REG_DIM6              0x2A
#define AW9523B_REG_DIM7              0x2B
#define AW9523B_REG_DIM8              0x20
#define AW9523B_REG_DIM9              0x21
#define AW9523B_REG_DIM10             0x22
#define AW9523B_REG_DIM11             0x23
#define AW9523B_REG_DIM12             0x2C
#define AW9523B_REG_DIM13             0x2D
#define AW9523B_REG_DIM14             0x2E
#define AW9523B_REG_DIM15             0x2F
#define AW9523B_REG_RESET             0x7F

#define AW9523B_MODE_LED_CURR_100     0x00
#define AW9523B_MODE_LED_CURR_75      0x01
#define AW9523B_MODE_LED_CURR_50      0x02
#define AW9523B_MODE_LED_CURR_25      0x03
#define AW9523B_MODE_GPIO             0x10

class AW9523B {
public:
    AW9523B(i2c_inst_t* i2c, uint8_t address = AW9523B_ADDRESS);
    bool begin(uint8_t mode);

    void setPinOutput(uint8_t pin);
    void setPinInput(uint8_t pin);
    void writePin(uint8_t pin, bool value);
    bool readPin(uint8_t pin);
    void setLEDBrightness(uint8_t pin, uint8_t brightness); // 0-255
    void enableLED(uint8_t pin);

    void forceLEDTestMode();
    // Move back to private if not needed
    bool writeRegister(uint8_t reg, uint8_t value);
    bool readRegister(uint8_t reg, uint8_t* value);
private:
    i2c_inst_t* _i2c;
    uint8_t _address;

};

#endif
