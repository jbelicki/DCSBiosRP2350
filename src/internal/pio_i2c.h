#ifndef __PIO_I2C_H
#define __PIO_I2C_H

#include "hardware/pio.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void pio_i2c_init(PIO pio, uint sm, uint offset, uint pin_scl, uint pin_sda);

int pio_i2c_write_blocking(PIO pio, uint sm, uint8_t addr, uint8_t *txbuf, uint len);
int pio_i2c_read_blocking(PIO pio, uint sm, uint8_t addr, uint8_t *rxbuf, uint len);

bool pio_i2c_check_error(PIO pio, uint sm);
void pio_i2c_resume_after_error(PIO pio, uint sm);
void pio_i2c_rx_enable(PIO pio, uint sm, bool en);

#ifdef __cplusplus
}
#endif

#endif
