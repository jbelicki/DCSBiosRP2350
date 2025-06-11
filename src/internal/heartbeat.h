#ifndef __DCSBIOS_HEARTBEAT_H
#define __DCSBIOS_HEARTBEAT_H

#include <stdint.h>

namespace DcsBios {
    void initHeartbeat(int pin);
    void updateHeartbeat();
}

#endif
