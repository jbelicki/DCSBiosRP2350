#include "Display.h"
#include "SegmentFont.h"
#include "pico/time.h"
#include "hardware/sync.h"
#include "string.h"

namespace DcsBios {

    Display::Display(Ht16k33* driver, uint16_t address, uint16_t mask, DisplayMode mode, uint8_t offset)
        : driver_(driver), address_(address), mask_(mask), mode_(mode), offset_(offset), refreshIntervalMs_(0) {
        memset(digits_, 0, sizeof(digits_));
        lastRefresh_ = get_absolute_time();
    }

    void Display::setRefreshInterval(uint32_t ms) {
        refreshIntervalMs_ = ms;
    }

    void Display::storeDigits(uint16_t value) {
        // Limit to 4 digits max
        for (int i = 3; i >= 0; i--) {
            digits_[i] = value % 10;
            value /= 10;
        }
    }

    void Display::updateDisplay() {
        for (int i = 0; i < 4; i++) {
            driver_->setRawDigit(i, digitToSegment[digits_[i]]);
        }
        driver_->writeDisplay();
    }

    void Display::onDcsBiosFrame(uint16_t addr, uint8_t* data, uint16_t len) {
        if (len < 2 || addr != address_) return;
        uint16_t raw = (data[0] << 8) | data[1];
        uint16_t masked = raw & mask_;

        switch (mode_) {
            case DisplayMode::Full4Digit: {
                storeDigits(masked);
                break;
            }
            case DisplayMode::TwoDigitSplit: {
                uint8_t val = (mask_ & 0xFF00) ? (masked >> 8) : (masked & 0xFF);
                digits_[offset_] = val / 10;
                digits_[offset_ + 1] = val % 10;
                break;
            }
            case DisplayMode::PerDigit: {
                uint8_t val = (mask_ & 0xFF00) ? (masked >> 8) : (masked & 0xFF);
                digits_[offset_] = val % 10; // Store only one digit
                break;
            }
        }

        updateDisplay();
    }

    void Display::loop() {
        if (refreshIntervalMs_ > 0 && absolute_time_diff_us(lastRefresh_, get_absolute_time()) > refreshIntervalMs_ * 1000) {
            updateDisplay();
            lastRefresh_ = get_absolute_time();
        }
    }

} // namespace DcsBios
