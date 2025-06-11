#ifndef __DCSBIOS_DISPLAY_H__
#define __DCSBIOS_DISPLAY_H__

#include "ht16k33a.h"
#include <stdint.h>

namespace DcsBios {

    enum class DisplayMode {
        Full4Digit,      // One address controls all 4 digits
        TwoDigitSplit,   // Two addresses control 2 digits each
        PerDigit         // Each digit has its own address
    };

    class Display {
    public:
        /**
         * Constructor for a Display instance
         * 
         * @param driver  Pointer to the Ht16k33 driver instance
         * @param address DCS-BIOS address to listen for
         * @param mask    Bitmask for filtering data (e.g., 0xFF00 or 0x00FF)
         * @param mode    Display mode (Full4Digit, TwoDigitSplit, PerDigit)
         * @param offset  Digit offset (0–3), used for TwoDigitSplit and PerDigit modes
         */
        Display(Ht16k33* driver, uint16_t address, uint16_t mask, DisplayMode mode, uint8_t offset = 0);

        /**
         * Called each time a new DCS-BIOS frame is parsed
         */
        void onDcsBiosFrame(uint16_t addr, uint8_t* data, uint16_t len);

        /**
         * Enables periodic re-writes of the display (optional)
         */
        void setRefreshInterval(uint32_t ms);

        /**
         * Call regularly (e.g., in main loop) if using refreshInterval
         */
        void loop();

    private:
        Ht16k33* driver_;
        DisplayMode mode_;
        uint16_t address_;
        uint16_t mask_;
        uint8_t offset_;           // Digit offset (0–3)
        uint8_t digits_[4];        // Stored display digits (0–9)
        uint32_t refreshIntervalMs_;
        absolute_time_t lastRefresh_;

        void updateDisplay();      // Push digits[] to display
        void storeDigits(uint16_t value); // Break value into digits_
    };

} // namespace DcsBios

#endif // __DCSBIOS_DISPLAY_H__
