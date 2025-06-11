#ifndef __DCSBIOS_BOARDMODE_H
#define __DCSBIOS_BOARDMODE_H

#include <stdint.h>

namespace DcsBios {

    enum class BoardModeType {
        HOST,
        SLAVE,
        USB_ONLY,
        RS485_TERMINAL,
        INVALID
    };

    struct BoardMode {
        uint8_t address;
        BoardModeType mode;
    };

    extern BoardMode currentBoardMode;
    BoardMode determineBoardMode(uint8_t address);

}

#endif // __DCSBIOS_BOARDMODE_H
