#include "matrix_display.hpp"
#include "PinNames.h"
#include "mbed.h"
#include "mbed_retarget.h"
#include <sys/_stdint.h>

#define max7219_reg_noop 0x00
#define max7219_reg_digit0 0x01
#define max7219_reg_digit1 0x02
#define max7219_reg_digit2 0x03
#define max7219_reg_digit3 0x04
#define max7219_reg_digit4 0x05
#define max7219_reg_digit5 0x06
#define max7219_reg_digit6 0x07
#define max7219_reg_digit7 0x08
#define max7219_reg_decodeMode 0x09
#define max7219_reg_intensity 0x0a
#define max7219_reg_scanLimit 0x0b
#define max7219_reg_shutdown 0x0c
#define max7219_reg_displayTest 0x0f

MatrixDisplay::MatrixDisplay(PinName mosi, PinName slck, PinName load_pin,
                             uint32_t frequency)
    : load(load_pin), spi(mosi, NC, slck) {

  spi.format(8, 0);

  spi.frequency(frequency); // down to 100khx easier to scope ;-)

  write(max7219_reg_scanLimit, 0x07);
  write(max7219_reg_decodeMode,
        0x00);                          // using an led matrix (not digits)
  write(max7219_reg_shutdown, 0x01);    // not in shutdown mode
  write(max7219_reg_displayTest, 0x00); // no display test
  for (int e = 1; e <= 8; e++) {        // empty registers, turn all LEDs off
    write(e, 0);
  }
  // maxAll(max7219_reg_intensity, 0x0f & 0x0f);    // the first 0x0f is the
  // value you can set
  write(max7219_reg_intensity, 0x08);
}

void MatrixDisplay::write(int reg, int col) {
  load = 0;
  spi.write(reg);
  spi.write(col);
  load = 1;
}

void MatrixDisplay::display_next(uint16_t reading) {
  // if (last_value == 0) { // speed down the reading
  //   last_value = reading;
  // } else {
    // uint8_t average = last_value + reading / 2;
    
    uint8_t level = reading >> 13;

    uint8_t pattern = 1 << level;

    buf[index] = pattern;

    index = (index + 1) % 8;

    this->send();

    last_value = 0;
  // }
}

void MatrixDisplay::send() {
  for (int i = 0; i < 8; i++) {
    int j = (i + index) % 8;

    write(i + 1, buf[j]);
  }
}

void MatrixDisplay::clear() {
  for (int e = 1; e <= 8; e++) { // empty registers, turn all LEDs off
    write(e, 0);
  }
}
