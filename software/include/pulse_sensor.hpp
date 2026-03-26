#ifndef SENSORS_H
#define SENSORS_H

#include "mbed.h"
#include <cstdint>

constexpr int BUFFER_LENGTH = 64;
constexpr int TIME_BUFFER_LENGTH = 4;

class PulseSensor {
    private:
        AnalogIn ain;
        Ticker t;
        bool ready_to_read;

        void set_ready();

        uint16_t data[BUFFER_LENGTH];
        int16_t cleaned_data[BUFFER_LENGTH];
        uint16_t data_index = 0;

        int16_t sensor_min = 0;
        int16_t sensor_max = 0;
    
        void compute_bounds();

        bool rising_edge = false;

        Timer pulse_timer;
        std::chrono::microseconds time_buffer[TIME_BUFFER_LENGTH];
        uint8_t time_buffer_index = 0;
        int64_t average_pulse = 0;

      public:
        PulseSensor(PinName name, std::chrono::microseconds rate);
        bool is_ready();
        uint16_t take_reading_u16();
        float get_heart_rate(uint16_t reading);
};

#endif

