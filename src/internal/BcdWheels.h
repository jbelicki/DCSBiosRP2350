#ifndef __DCSBIOS_BCDWHEELS_H
#define __DCSBIOS_BCDWHEELS_H

#include "pico/stdlib.h"
#include "PollingInput.h"
#include "ExportStreamListener.h"

namespace DcsBios {

template <unsigned long pollIntervalMs = POLL_EVERY_TIME>
class BcdWheelT : public PollingInput, public ResettableInput {
private:
    const char* msg_;
    uint pinA_, pinB_, pinC_, pinD_;
    char lastState_;

    char readState() {
        int total = 0;
        if (gpio_get(pinA_) == 0) total += 1;
        if (gpio_get(pinB_) == 0) total += 2;
        if (pinC_ != 255 && gpio_get(pinC_) == 0) total += 4;
        if (pinD_ != 255 && gpio_get(pinD_) == 0) total += 8;
        return total;
    }

    void resetState() {
        lastState_ = (lastState_ == 0) ? -1 : 0;
    }

    void pollInput() {
        char state = readState();
        if (state != lastState_) {
            char szBody[2] = { static_cast<char>(state + '0'), 0 };
            if (tryToSendDcsBiosMessage(msg_, szBody)) lastState_ = state;
        }
    }

public:
    BcdWheelT(const char* msg, uint pinA, uint pinB, uint pinC = 255, uint pinD = 255) :
        PollingInput(pollIntervalMs), msg_(msg),
        pinA_(pinA), pinB_(pinB), pinC_(pinC), pinD_(pinD)
    {
        gpio_init(pinA_); gpio_pull_up(pinA_);
        gpio_init(pinB_); gpio_pull_up(pinB_);
        if (pinC_ != 255) { gpio_init(pinC_); gpio_pull_up(pinC_); }
        if (pinD_ != 255) { gpio_init(pinD_); gpio_pull_up(pinD_); }
        lastState_ = readState();
    }

    void resetThisState() { this->resetState(); }
};
typedef BcdWheelT<> BcdWheel;

template <unsigned long pollIntervalMs = POLL_EVERY_TIME>
class RadioPresetT : public PollingInput, public ResettableInput {
private:
    const char* msg_;
    uint pinA_, pinB_, pinC_, pinD_, pinE_;
    char lastState_;

    char readState() {
        int total = 0;
        if (gpio_get(pinA_) == 0) total += 1;
        if (gpio_get(pinB_) == 0) total += 2;
        if (gpio_get(pinC_) == 0) total += 4;
        if (gpio_get(pinD_) == 0) total += 8;
        if (gpio_get(pinE_) == 0) total += 16;
        return total;
    }

    void resetState() {
        lastState_ = (lastState_ == 0) ? -1 : 0;
    }

    void pollInput() {
        static const char* values[] = {
            "0", "0.05", "0.1", "0.15", "0.2", "0.25", "0.3", "0.35",
            "0.4", "0.45", "0.5", "0.55", "0.6", "0.65", "0.7", "0.75",
            "0.8", "0.85", "0.9", "0.95", "1"
        };
        char state = readState();
        if (state != lastState_ && state < 21) {
            if (tryToSendDcsBiosMessage(msg_, values[state])) {
                lastState_ = state;
            }
        }
    }

public:
    RadioPresetT(const char* msg, uint pinA, uint pinB, uint pinC, uint pinD, uint pinE) :
        PollingInput(pollIntervalMs),
        msg_(msg), pinA_(pinA), pinB_(pinB), pinC_(pinC), pinD_(pinD), pinE_(pinE)
    {
        gpio_init(pinA_); gpio_pull_up(pinA_);
        gpio_init(pinB_); gpio_pull_up(pinB_);
        gpio_init(pinC_); gpio_pull_up(pinC_);
        gpio_init(pinD_); gpio_pull_up(pinD_);
        gpio_init(pinE_); gpio_pull_up(pinE_);
        lastState_ = readState();
    }

    void resetThisState() { this->resetState(); }
};
typedef RadioPresetT<> RadioPreset;

} // namespace DcsBios

#endif
