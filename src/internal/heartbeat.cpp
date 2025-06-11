#include "heartbeat.h"
#include "pico/stdlib.h"

namespace DcsBios {
    static uint heartbeatPin = 0;
    static bool heartbeatState = false;
    static uint32_t lastHeartbeat = 0;

    void initHeartbeat(int pin) {
        heartbeatPin = pin;
        gpio_init(heartbeatPin);
        gpio_set_dir(heartbeatPin, GPIO_OUT);
        gpio_put(heartbeatPin, 0); // Ensure off at startup
    }

    void updateHeartbeat() {
        const uint32_t interval = 500; // ms
        uint32_t now = to_ms_since_boot(get_absolute_time());

        if (now - lastHeartbeat >= interval) {
            heartbeatState = !heartbeatState;
            gpio_put(heartbeatPin, heartbeatState);
            lastHeartbeat = now;
        }
    }
}
