
#ifndef MATRIX_H
#define MATRIX_H

#include "DigitalOut.h"
#include "PinNames.h"
#include "mbed.h"
#include <cstdint>

class MatrixDisplay {
    private:
        DigitalOut load;
        SPI spi;

        char buf[8] = {};
        uint8_t index = 0;

        uint8_t readtime = 0;
        uint8_t queued_value = 0;
        

        void write(int reg, int col);

        void send();
      public:
        MatrixDisplay (PinName mosi, PinName slck, PinName load, uint32_t frequency);
        bool is_ready();
        void display_next(uint16_t reading);
        void clear();
};

#endif

