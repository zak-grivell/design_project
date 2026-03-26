
/* mbed Microcontroller Library
 * Copyright (c) 2026 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "PwmOut.h"
#include "matrix_display.hpp"
#include "mbed.h"
#include <cstdint>
#include "pulse_sensor.hpp"
#include "TextLCD.h"

PulseSensor pulse_sensor(p19, 20ms);
MatrixDisplay mat(p5, p7, p8, 100000);
DigitalIn led_switch(p22);
PwmOut led(p21);
TextLCD text_lcd(p30, p29, p28, p27, p26, p25);

int main() {
    led.pulsewidth(0.001); // 1ms
    
    while (true) {
        if (pulse_sensor.is_ready()) {
            uint16_t reading = pulse_sensor.take_reading_u16();

            // if (led_switch) {
                // led = 0;
                float heart_rate = pulse_sensor.get_heart_rate(reading);
                text_lcd.printf("Heart Rate=%d\n\n", (int)(heart_rate));
            // } else {
                led.write((float)reading / 65535);
                // text_lcd.printf("\n");
            // }

            mat.display_next(reading);
        }
    }
}
