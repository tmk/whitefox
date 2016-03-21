/*
Copyright 2016 flabbergast <s3+flabbergast@sdfeu.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * LED controller code
 * WF uses IS31FL3731C matrix LED driver from ISSI
 * datasheet: http://www.issi.com/WW/pdf/31FL3731C.pdf
 */

#include "ch.h"
#include "hal.h"

#include "led_controller.h"

#include <string.h> // memcpy

/* WF LED MAP
    - digits mean "row" and "col", i.e. 45 means C4-5 in the ISSI datasheet, matrix A

  11 12 13 14 15 16 17 18 21 22 23 24 25 26 27  28
   31 32 33 34 35 36 37 38 41 42 43 44 45  46   47
   48 51 52 53 54 55 56 57 58 61 62 63 64   65  66
    67 68 71 72 73 74 75 76 77 78 81 82  83  84 85
  86  87  88       91        92  93 (94)  95 96 97
*/

/*
  each page has 0xB4 bytes
  0 - 0x11: LED control (on/off):
    order: CA1, CB1, CA2, CB2, .... (CA - matrix A, CB - matrix B)
      CAn controls Cn-8 .. Cn-1 (LSbit)
  0x12 - 0x23: blink control (like "LED control")
  0x24 - 0xB3: PWM control: byte per LED, 0xFF max on
    order same as above (CA 1st row (8bytes), CB 1st row (8bytes), ...)
*/

/* =================
 * ChibiOS I2C setup
 * ================= */
static const I2CConfig i2ccfg = {
  400000 // clock speed (Hz); 400kHz max for ISSI
};

/* ==============
 *   variables
 * ============== */
// basic communication buffers
uint8_t tx[2] __attribute__((aligned(4)));
uint8_t rx[1] __attribute__((aligned(4)));

// buffer for sending the whole page at once
uint8_t full_page[0xB4+1] = {0};

// LED mask (which LEDs are present, selected by bits)
const uint8_t issi_leds_mask[0x12] = {
  0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF,
  0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x7F, 0x00
};

/* ============================
 *   communication functions
 * ============================ */
msg_t issi_select_page(uint8_t page) {
  tx[0] = ISSI_COMMANDREGISTER;
  tx[1] = page;
  return i2cMasterTransmitTimeout(&I2CD1, ISSI_ADDR_DEFAULT, tx, 2, rx, 1, US2ST(ISSI_TIMEOUT));
}

msg_t issi_write_data(uint8_t page, uint8_t *buffer, uint8_t size) {
  issi_select_page(page);
  return i2cMasterTransmitTimeout(&I2CD1, ISSI_ADDR_DEFAULT, buffer, size, rx, 1, US2ST(ISSI_TIMEOUT));
}

msg_t issi_write_register(uint8_t page, uint8_t reg, uint8_t data) {
  issi_select_page(page);
  tx[0] = reg;
  tx[1] = data;
  return i2cMasterTransmitTimeout(&I2CD1, ISSI_ADDR_DEFAULT, tx, 2, rx, 1, US2ST(ISSI_TIMEOUT));
}

// read value in rx[0]
msg_t issi_read_register(uint8_t b, uint8_t reg) {
  issi_select_page(b);

  tx[0] = reg;
  return i2cMasterTransmitTimeout(&I2CD1, ISSI_ADDR_DEFAULT, tx, 1, rx, 1, US2ST(ISSI_TIMEOUT));
}

/* ========================
 * initialise the ISSI chip
 * ======================== */
void issi_init(void) {
  // zero function page, all registers (assuming full_page is all zeroes)
  issi_write_data(ISSI_FUNCTIONREG, full_page, 0xD + 1);
  // disable hardware shutdown
  palSetPadMode(GPIOB, 16, PAL_MODE_OUTPUT_PUSHPULL);
  palSetPad(GPIOB, 16);
  chThdSleepMilliseconds(10);
  // software shutdown
  issi_write_register(ISSI_FUNCTIONREG, ISSI_REG_SHUTDOWN, 0);
  chThdSleepMilliseconds(10);
  // zero function page, all registers
  issi_write_data(ISSI_FUNCTIONREG, full_page, 0xD + 1);
  chThdSleepMilliseconds(10);
  // software shutdown disable (i.e. turn stuff on)
  issi_write_register(ISSI_FUNCTIONREG, ISSI_REG_SHUTDOWN, ISSI_REG_SHUTDOWN_ON);
  chThdSleepMilliseconds(10);
  // zero all LED registers on all 8 pages
  uint8_t i;
  for(i=0; i<8; i++) {
    issi_write_data(i, full_page, 0xB4 + 1);
    chThdSleepMilliseconds(1);
  }
}

/* ==================
 * LED control thread
 * ================== */
// thread_reference_t trp = NULL; // thread reference (for waiting for events)
// static THD_WORKING_AREA(waLEDthread, 32);
// static THD_FUNCTION(LEDthread, arg) {
//   (void)arg;
//   chRegSetThreadName("LEDthread");

//   msg_t msg;

//   while(true) {
//     // wait for a message (synchronous)
//     chSysLock();
//     msg = chThdSuspend(&trp);
//     chSysUnlock();

//     // process 'msg' here
//     switch(msg) {
//       case LED_CTR_CAPS_ON:
//         issi_write_register(0, 0x5B, 0xFF); // full brightness
//         break;
//       case LED_CTR_CAPS_OFF:
//         issi_write_register(0, 0x5B, 0x00);
//         break;
//     }
//   }
// }

/* =============
 * hook into TMK
 * ============= */
void early_init_hook(void) {
  /* initialise I2C */
  /* I2C pins */
  palSetPadMode(GPIOB, 0, PAL_MODE_ALTERNATIVE_2); // PTB0/I2C0/SCL
  palSetPadMode(GPIOB, 1, PAL_MODE_ALTERNATIVE_2); // PTB1/I2C0/SDA
  /* start I2C */
  i2cStart(&I2CD1, &i2ccfg);
  // try high drive (from kiibohd)
  I2CD1.i2c->C2 |= I2Cx_C2_HDRS;
  // try glitch fixing (from kiibohd)
  I2CD1.i2c->FLT = 4;

  chThdSleepMilliseconds(10);

  /* initialise ISSI chip */
  issi_init();

  /* enable WF LEDs on page 0 */
  full_page[0] = 0;
  memcpy(full_page+1, issi_leds_mask, 0x12);
  issi_write_data(0, full_page, 1+0x12);

  /* more time consuming LED processing should be offloaded into
   * a thread, with asynchronous messaging. */
  // chThdCreateStatic(waLEDthread, sizeof(waLEDthread), LOWPRIO, LEDthread, NULL);
}
