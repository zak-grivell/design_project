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
  for (uint16_t y : data) {
    sum += y;
  }

  int32_t average = sum / BUFFER_LENGTH;

  for (int i = 0; i < BUFFER_LENGTH; i++) {
    cleaned_data[i] = data[i] - average;
  }

  sensor_min = 0;
  sensor_max = 0;

  for (int16_t y : cleaned_data) {
    if (y < sensor_min)
      sensor_min = y;
    if (y > sensor_max)
      sensor_max = y;
  }
}

uint16_t PulseSensor::take_reading_u16() {
  uint8_t prev = (data_index == 0) ? (BUFFER_LENGTH - 1) : (data_index - 1);
  data[data_index] = (ain.read_u16() + data[prev]) / 2;

  compute_bounds();

  if (sensor_max == sensor_min)
    return 0;

  uint16_t scaled = ((cleaned_data[data_index] - sensor_min) * 65535) /
                    (sensor_max - sensor_min);
  data_index = (data_index + 1) % BUFFER_LENGTH;

  return scaled;
}

float PulseSensor::get_heart_rate(uint16_t reading) {
  if (this->rising_edge && reading < LOWER_TRIGGER) {
    this->rising_edge = false;
  } else if (!this->rising_edge && reading > UPPER_TRIGGER) {
    this->rising_edge = true;

    std::chrono::microseconds time = this->pulse_timer.elapsed_time();
    this->pulse_timer.reset();
    this->time_buffer[time_buffer_index] = time;

    time_buffer_index =
        (time_buffer_index + 1) % TIME_BUFFER_LENGTH;

    uint64_t total = 0;

    for (int i = 0; i < TIME_BUFFER_LENGTH; i++) {
      total += this->time_buffer[i].count();
    }

    float avg_interval = (float)total / (float)TIME_BUFFER_LENGTH;
    this->average_pulse = 60'000'000.0f / avg_interval;
  }

  return this->average_pulse;
}
