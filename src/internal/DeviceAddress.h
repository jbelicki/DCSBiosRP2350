#ifndef __DEVICE_ADDRESS_H
#define __DEVICE_ADDRESS_H

#include <stdint.h>

namespace DcsBios {

    void initDeviceAddressPins(uint8_t pin0, uint8_t pin1, uint8_t pin2, uint8_t pin3);
    uint8_t readDeviceAddress();

}

#endif // __DEVICE_ADDRESS_H
