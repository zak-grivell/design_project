#include "pulse_sensor.hpp"
#include "mbed.h"
#include <cstdint>

PulseSensor::PulseSensor(PinName name, std::chrono::microseconds rate)
    : ain(name) {
  t.attach(callback(this, &PulseSensor::set_ready), rate);
  ready_to_read = false;
  this->pulse_timer.start();
}

void PulseSensor::set_ready() { ready_to_read = true; }

bool PulseSensor::is_ready() {
  if (ready_to_read) {
    ready_to_read = false;
    return true;
  }
  return false;
}

void PulseSensor::compute_bounds() {
    uint32_t sum = 0;
    for (uint16_t y : data) sum += y;
    int32_t average = sum / BUFFER_LENGTH;

    for (int i = 0; i < BUFFER_LENGTH; i++)
        cleaned_data[i] = (int16_t)(data[i] - average);

    int64_t sum_sq = 0;
    for (int16_t y : cleaned_data) sum_sq += (int32_t)y * y;
    int32_t std_dev = (int32_t)sqrtf((float)(sum_sq / BUFFER_LENGTH));

    sensor_min = -2 * std_dev;
    sensor_max =  2 * std_dev;
}

uint16_t PulseSensor::take_reading_u16() {
    uint16_t raw = ain.read_u16();

    if (iir_state == 0.0f) iir_state = (float)raw;
    iir_state = 0.25f * (float)raw + 0.75f * iir_state;
    data[data_index] = (uint16_t)iir_state;

    compute_bounds();
    if (sensor_max <= 0) return 0; // flat signal, no pulse detected

    int32_t range = sensor_max - sensor_min;
    int32_t clamped = cleaned_data[data_index];
    if (clamped < sensor_min) clamped = sensor_min;
    if (clamped > sensor_max) clamped = sensor_max;

    uint16_t scaled = (uint16_t)(((clamped - sensor_min) * 65535L) / range);
    data_index = (data_index + 1) % BUFFER_LENGTH;
    return scaled;
}

float PulseSensor::get_heart_rate(uint16_t reading) {
    if (this->rising_edge && reading < LOWER_TRIGGER) {
        this->rising_edge = false;
    } else if (!this->rising_edge && reading > UPPER_TRIGGER) {
        this->rising_edge = true;
        std::chrono::microseconds interval = this->pulse_timer.elapsed_time();
        this->pulse_timer.reset();

        // Plausibility gate: 40–200 BPM = 300ms–1500ms
        if (interval.count() > 300000 && interval.count() < 1500000) {
            this->time_buffer[time_buffer_index] = interval;
            time_buffer_index = (time_buffer_index + 1) % TIME_BUFFER_LENGTH;
            if (valid_beats < TIME_BUFFER_LENGTH) valid_beats++;

            if (valid_beats == TIME_BUFFER_LENGTH) {
                uint64_t total = 0;
                for (int i = 0; i < TIME_BUFFER_LENGTH; i++)
                    total += this->time_buffer[i].count();
                float avg_interval = (float)total / (float)TIME_BUFFER_LENGTH;
                this->average_pulse = 60'000'000.0f / avg_interval;
            }
        }
    }
    return this->average_pulse;
}
