
/* mbed Microcontroller Library
 * Copyright (c) 2026 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "TextLCD.h"
#include "Timer.h"
#include "matrix_display.hpp"
#include "mbed.h"
#include "pulse_sensor.hpp"
#include <cstdint>

PulseSensor pulse_sensor(p19, 20ms);
MatrixDisplay mat(p5, p7, p8, 100000);
DigitalIn led_switch(p22);
PwmOut led(p21);
TextLCD text_lcd(p30, p29, p28, p27, p26, p25);

int main() {
  led.pulsewidth(0.001);

  while (true) {
    if (pulse_sensor.is_ready()) {
      uint16_t reading = pulse_sensor.take_reading_u16();

      if (led_switch) {
        led = 0;
        float heart_rate = pulse_sensor.get_heart_rate(reading);

        text_lcd.locate(0, 0);
        text_lcd.printf("HR: %3d BPM ", (int)heart_rate);
      } else {
        led.write((float)reading / 65535);
        // text_lcd.printf("\n\n");
      }
      
      mat.display_next(reading);
    }
  }
}
