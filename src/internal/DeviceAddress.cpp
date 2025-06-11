#include "DeviceAddress.h"
#include "pico/stdlib.h"

namespace DcsBios {

// Internal static storage
static uint8_t addressPins[4];

void initDeviceAddressPins(uint8_t pin0, uint8_t pin1, uint8_t pin2, uint8_t pin3) {
    addressPins[0] = pin0;
    addressPins[1] = pin1;
    addressPins[2] = pin2;
    addressPins[3] = pin3;

    for (int i = 0; i < 4; i++) {
        gpio_init(addressPins[i]);
        gpio_set_dir(addressPins[i], GPIO_IN);
        gpio_pull_up(addressPins[i]);
    }
}

uint8_t readDeviceAddress() {
    uint8_t addr = 0;
    for (int i = 0; i < 4; i++) {
        addr |= (!gpio_get(addressPins[i])) << i; // active low
    }
    return addr;
}

}
