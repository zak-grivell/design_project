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
               0x00);                       // using an led matrix (not digits)
  write(max7219_reg_shutdown, 0x01); // not in shutdown mode
  write(max7219_reg_displayTest, 0x00); // no display test
  for (int e = 1; e <= 8; e++) { // empty registers, turn all LEDs off
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

char pattern_diagonal[8] = {0x01, 0x2, 0x4, 0x08, 0x10, 0x20, 0x40, 0x80};
char pattern_square[8] = {0xff, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xff};
char pattern_star[8] = {0x04, 0x15, 0x0e, 0x1f, 0x0e, 0x15, 0x04, 0x00};


// // writes 8 bytes to the display
// void pattern_to_display(char *testdata) {
//   int cdata;
//   for (int idx = 0; idx <= 7; idx++) {
//     cdata = testdata[idx];
//     write_to_max(idx + 1, cdata);
//   }
// }
//
//
void MatrixDisplay::display_next(uint16_t reading) {
    uint8_t level = reading >> 13;

    uint8_t pattern = 1 << level;

    buf[index] = pattern;

    index++;
}

void MatrixDisplay::send() {
  for (int i = 0; i < 8; i++) {
    int j = (i + index) % 8;

    write(j, buf[j]);
  }
}

void MatrixDisplay::clear() {
  for (int e = 1; e <= 8; e++) { // empty registers, turn all LEDs off
    write(e, 0);
  }
}
