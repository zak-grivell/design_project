
/* mbed Microcontroller Library
 * Copyright (c) 2026 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mbed.h"
#include <cstdint>

#define BUFFER_LENGTH 128

class PulseSensor {
    private:
        AnalogIn ain;
        Ticker t;
        bool reader;

        void set_ready() {
            reader = true;
        }

        uint16_t data[BUFFER_LENGTH];
        uint16_t data_index = 0;

        int16_t sensor_min = 0;
        int16_t sensor_max = 0;
        int16_t average = 0;

        void compute_bounds() {
            uint32_t sum = 0;
            for (uint32_t y: data) {
                sum += y;

                if (y < sensor_min) {
                    sensor_min = y;
                }

                if (y > sensor_max) {
                    sensor_max = y;
                }
            }

            average = sum / BUFFER_LENGTH;
        }
    public:
        PulseSensor(PinName name, std::chrono::microseconds rate): ain(name) {
            t.attach(callback(this, &PulseSensor::set_ready), 5ms);
        }

        bool is_ready() {
            if (reader) {
                reader = false;
                return true;
            }
            return false;
        }

        uint16_t take_reading_u16() {
            uint8_t previous_index = data_index == 0 ? 127 : (data_index - 1);

            uint16_t value = (ain.read_u16() + data[previous_index]) / 2;
            
            data[data_index % 128] = value;

            data_index = (data_index + 1) % 128;

            uint16_t scaled = ((value - sensor_min) * 65535) / (sensor_max - sensor_min);

            return scaled;
        }

};


uint16_t to_matrix_level(uint16_t scaled) {
    return scaled & 0xFF00;
}

PulseSensor pulse_sensor(p19, 5ms);
AnalogOut aout(p18);

int main() {
    while (true) {
        if (pulse_sensor.is_ready()) {
            uint16_t reading = pulse_sensor.take_reading_u16();

            uint16_t matrix_level = to_matrix_level(reading);
            
            aout.write_u16(matrix_level);
        }
    }
}
