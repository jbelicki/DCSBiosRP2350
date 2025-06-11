#include "BoardMode.h"
#include "FoxConfig.h"  // for HOST_MODE, SLAVE_MODE_MIN, etc.

namespace DcsBios {

BoardMode currentBoardMode; 

BoardMode determineBoardMode(uint8_t address) {
    BoardMode bm;
    bm.address = address;

    if (address == HOST_MODE) {
        bm.mode = BoardModeType::HOST;
    } else if (address == USB_MODE) {
        bm.mode = BoardModeType::USB_ONLY;
    } else if (address == RS485_TERMINAL_MODE) {
        bm.mode = BoardModeType::RS485_TERMINAL;
    } else if (address >= SLAVE_MODE_MIN && address <= SLAVE_MODE_MAX) {
        bm.mode = BoardModeType::SLAVE;    
    } else {
        bm.mode = BoardModeType::INVALID;
    }

    return bm;
}

}
