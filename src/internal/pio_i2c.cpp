/**
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

 #include "pio_i2c.h"
 #include "i2c.pio.h"
 #include "hardware/clocks.h"
 #include "hardware/gpio.h"
 #include "pico/stdlib.h"
 
 const int PIO_I2C_ICOUNT_LSB = 10;
 const int PIO_I2C_FINAL_LSB  = 9;
 const int PIO_I2C_DATA_LSB   = 1;
 const int PIO_I2C_NAK_LSB    = 0;
 
 // --- Added initialization for swapped pin order ---
 void pio_i2c_init(PIO pio, uint sm, uint offset, uint pin_scl, uint pin_sda) {
     // Your board must have SDA = SCL + 1
     assert(pin_sda == pin_scl + 1);
 
     pio_sm_config c = i2c_program_get_default_config(offset);
 
     sm_config_set_out_pins(&c, pin_sda, 1);
     sm_config_set_set_pins(&c, pin_sda, 1);
     sm_config_set_in_pins(&c, pin_sda);
     sm_config_set_jmp_pin(&c, pin_sda);
     sm_config_set_sideset_pins(&c, pin_scl);
 
     sm_config_set_out_shift(&c, false, true, 16);
     sm_config_set_in_shift(&c, false, true, 8);
 
     float div = (float)clock_get_hz(clk_sys) / (32 * 100000);
     sm_config_set_clkdiv(&c, div);
 
     gpio_pull_up(pin_scl);
     gpio_pull_up(pin_sda);
     pio_gpio_init(pio, pin_scl);
     gpio_set_oeover(pin_scl, GPIO_OVERRIDE_INVERT);
     pio_gpio_init(pio, pin_sda);
     gpio_set_oeover(pin_sda, GPIO_OVERRIDE_INVERT);
 
     uint32_t both_pins = (1u << pin_scl) | (1u << pin_sda);
     pio_sm_set_pins_with_mask(pio, sm, 0, both_pins);
     pio_sm_set_pindirs_with_mask(pio, sm, both_pins, both_pins);
 
     pio_set_irq0_source_enabled(pio, (enum pio_interrupt_source)(pis_interrupt0 + sm), false);
     pio_set_irq1_source_enabled(pio, (enum pio_interrupt_source)(pis_interrupt0 + sm), false);
     pio_interrupt_clear(pio, sm);
 
     pio_sm_init(pio, sm, offset + i2c_offset_entry_point, &c);
     pio_sm_set_enabled(pio, sm, true);
 }
 
 // --- Existing I2C helpers ---
 
 bool pio_i2c_check_error(PIO pio, uint sm) {
     return pio_interrupt_get(pio, sm);
 }
 
 void pio_i2c_resume_after_error(PIO pio, uint sm) {
     pio_sm_drain_tx_fifo(pio, sm);
     pio_sm_exec(pio, sm, (pio->sm[sm].execctrl & PIO_SM0_EXECCTRL_WRAP_BOTTOM_BITS) >> PIO_SM0_EXECCTRL_WRAP_BOTTOM_LSB);
     pio_interrupt_clear(pio, sm);
 }
 
 void pio_i2c_rx_enable(PIO pio, uint sm, bool en) {
     if (en)
         hw_set_bits(&pio->sm[sm].shiftctrl, PIO_SM0_SHIFTCTRL_AUTOPUSH_BITS);
     else
         hw_clear_bits(&pio->sm[sm].shiftctrl, PIO_SM0_SHIFTCTRL_AUTOPUSH_BITS);
 }
 
 static inline void pio_i2c_put16(PIO pio, uint sm, uint16_t data) {
     while (pio_sm_is_tx_fifo_full(pio, sm))
         ;
     *(io_rw_16 *)&pio->txf[sm] = data;
 }
 
 void pio_i2c_put_or_err(PIO pio, uint sm, uint16_t data) {
     while (pio_sm_is_tx_fifo_full(pio, sm))
         if (pio_i2c_check_error(pio, sm))
             return;
     if (pio_i2c_check_error(pio, sm))
         return;
     *(io_rw_16 *)&pio->txf[sm] = data;
 }
 
 uint8_t pio_i2c_get(PIO pio, uint sm) {
     return (uint8_t)pio_sm_get(pio, sm);
 }
 
 void pio_i2c_start(PIO pio, uint sm) {
     pio_i2c_put_or_err(pio, sm, 1u << PIO_I2C_ICOUNT_LSB);
     pio_i2c_put_or_err(pio, sm, set_scl_sda_program_instructions[I2C_SC1_SD0]);
     pio_i2c_put_or_err(pio, sm, set_scl_sda_program_instructions[I2C_SC0_SD0]);
 }
 
 void pio_i2c_stop(PIO pio, uint sm) {
     pio_i2c_put_or_err(pio, sm, 2u << PIO_I2C_ICOUNT_LSB);
     pio_i2c_put_or_err(pio, sm, set_scl_sda_program_instructions[I2C_SC0_SD0]);
     pio_i2c_put_or_err(pio, sm, set_scl_sda_program_instructions[I2C_SC1_SD0]);
     pio_i2c_put_or_err(pio, sm, set_scl_sda_program_instructions[I2C_SC1_SD1]);
 }
 
 void pio_i2c_repstart(PIO pio, uint sm) {
     pio_i2c_put_or_err(pio, sm, 3u << PIO_I2C_ICOUNT_LSB);
     pio_i2c_put_or_err(pio, sm, set_scl_sda_program_instructions[I2C_SC0_SD1]);
     pio_i2c_put_or_err(pio, sm, set_scl_sda_program_instructions[I2C_SC1_SD1]);
     pio_i2c_put_or_err(pio, sm, set_scl_sda_program_instructions[I2C_SC1_SD0]);
     pio_i2c_put_or_err(pio, sm, set_scl_sda_program_instructions[I2C_SC0_SD0]);
 }
 
 static void pio_i2c_wait_idle(PIO pio, uint sm) {
     pio->fdebug = 1u << (PIO_FDEBUG_TXSTALL_LSB + sm);
     while (!(pio->fdebug & (1u << (PIO_FDEBUG_TXSTALL_LSB + sm))) && !pio_i2c_check_error(pio, sm))
         tight_loop_contents();
 }
 
 int pio_i2c_write_blocking(PIO pio, uint sm, uint8_t addr, uint8_t *txbuf, uint len) {
     int err = 0;
     pio_i2c_start(pio, sm);
     pio_i2c_rx_enable(pio, sm, false);
     pio_i2c_put16(pio, sm, (addr << 2) | 1u);
     while (len && !pio_i2c_check_error(pio, sm)) {
         if (!pio_sm_is_tx_fifo_full(pio, sm)) {
             --len;
             pio_i2c_put_or_err(pio, sm, (*txbuf++ << PIO_I2C_DATA_LSB) | ((len == 0) << PIO_I2C_FINAL_LSB) | 1u);
         }
     }
     pio_i2c_stop(pio, sm);
     pio_i2c_wait_idle(pio, sm);
     if (pio_i2c_check_error(pio, sm)) {
         err = -1;
         pio_i2c_resume_after_error(pio, sm);
         pio_i2c_stop(pio, sm);
     }
     return err;
 }
 
 int pio_i2c_read_blocking(PIO pio, uint sm, uint8_t addr, uint8_t *rxbuf, uint len) {
     int err = 0;
     pio_i2c_start(pio, sm);
     pio_i2c_rx_enable(pio, sm, true);
     while (!pio_sm_is_rx_fifo_empty(pio, sm))
         (void)pio_i2c_get(pio, sm);
 
     pio_i2c_put16(pio, sm, (addr << 2) | 3u);
 
     uint32_t tx_remain = len;
     bool first = true;
 
     while ((tx_remain || len) && !pio_i2c_check_error(pio, sm)) {
         if (tx_remain && !pio_sm_is_tx_fifo_full(pio, sm)) {
             --tx_remain;
             pio_i2c_put16(pio, sm, (0xffu << 1) | (tx_remain ? 0 : ((1u << PIO_I2C_FINAL_LSB) | (1u << PIO_I2C_NAK_LSB))));
         }
         if (!pio_sm_is_rx_fifo_empty(pio, sm)) {
             if (first) {
                 (void)pio_i2c_get(pio, sm);
                 first = false;
             } else {
                 --len;
                 *rxbuf++ = pio_i2c_get(pio, sm);
             }
         }
     }
 
     pio_i2c_stop(pio, sm);
     pio_i2c_wait_idle(pio, sm);
 
     if (pio_i2c_check_error(pio, sm)) {
         err = -1;
         pio_i2c_resume_after_error(pio, sm);
         pio_i2c_stop(pio, sm);
     }
 
     return err;
 }
 