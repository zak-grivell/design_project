
/* mbed Microcontroller Library
 * Copyright (c) 2026 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "matrix_display.hpp"
#include "mbed.h"
#include <cstdint>
#include "pulse_sensor.hpp"

uint16_t to_matrix_level(uint16_t scaled) {
    return scaled & 0xE000;
}

PulseSensor pulse_sensor(p19, 20ms);
AnalogOut aout(p18);
MatrixDisplay mat(p5, p7, p8, 100000);

int main() {
    while (true) {
        if (pulse_sensor.is_ready()) {
            uint16_t reading = pulse_sensor.take_reading_u16();

            mat.display_next(reading);
        }
    }
}
