
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
        int16_t cleaned_data[BUFFER_LENGTH];
        uint16_t data_index = 0;

        int16_t sensor_min = 0;
        int16_t sensor_max = 0;
    
        void compute_bounds() {
            uint32_t sum = 0;

            for (uint32_t y: data) {
                sum += y;
            }

            int16_t average = sum / BUFFER_LENGTH;

            for (int i = 0; i < BUFFER_LENGTH; i++) {
                cleaned_data[i] = data[i] - average;
            }

            sensor_min = 0;
            sensor_max = 0;

            for (int32_t y: cleaned_data) { 
                if (y < sensor_min) {
                    sensor_min = y;
                }

                if (y > sensor_max) {
                    sensor_max = y;
                }
            }
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
            uint8_t previous_index = data_index == 0 ? (BUFFER_LENGTH - 1) : (data_index - 1);

            uint16_t value = (ain.read_u16() + data[previous_index]) / 2;

            data[data_index] = value;

            compute_bounds();

            uint16_t scaled = ((cleaned_data[data_index] - sensor_min) * 65535) / (sensor_max - sensor_min);

            data_index = (data_index + 1) % BUFFER_LENGTH;

            return scaled;
        }

};


uint16_t to_matrix_level(uint16_t scaled) {
    return scaled & 0xE000;
}

PulseSensor pulse_sensor(p19, 20ms);
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
