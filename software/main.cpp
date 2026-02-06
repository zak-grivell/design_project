
/* mbed Microcontroller Library
 * Copyright (c) 2026 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mbed.h"

CircularBuffer<int, 128> raw_data;

CircularBuffer<int, 64> data;

int raw_sum;
int negative_sum = 0;
int positive_sum = 0;
int previous;

int apply_processing(int reading) {
    if (raw_data.full()) {
        int oldest_value;
        raw_data.pop(oldest_value);
        raw_sum -= oldest_value;
    }

    int filtered = (reading + previous) / 2;
    raw_sum += filtered;
    raw_data.push(filtered);

    previous = filtered;

    int y = filtered - (raw_sum / raw_data.size());

    data.push(y);

    return y;
}


// determines the min and max of each period to ensure propper capture even when the value heights change
int sensor_min = 0;
int sensor_max = 0;

void determine_bounds(int reading) {
    for (int i = 0; i < 128; i++) {
        int val;
        data.pop(val);
        data.push(val);

        if (val > sensor_max) {
            sensor_max = val;
        }

        if (val < sensor_min) {
            sensor_min = val;
        }
    }
}

// PulseSensor pulse_sensor(p19, 5ms);
AnalogOut aout(p18);
AnalogIn ain(p19);
Ticker t;
bool ready = false;

void set_ready() {
    ready = true;
}

int make_3bit_reading() {
    int reading = (int)ain.read_u16();

    reading = apply_processing(reading);

    determine_bounds(reading);

    float norm = ((float)reading - (float)sensor_min) / ((float)sensor_max - (float)sensor_min);

    if (norm < 0) {
        norm = 0;
    } else if (norm > 1) {
        norm = 1;
    }

    int three_bit = norm * 7.0f + 0.25f;

    int scaled = three_bit * 9362;

    return scaled;
}


int main() {
    t.attach(&set_ready, 5ms);
    while (true) {
        if (ready) {
            ready = false;
            int byte_val = make_3bit_reading();

            aout.write_u16(byte_val);
        }
    }
}
