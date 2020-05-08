#pragma once

#include <stdint.h>

class nibble_writer {
    uint8_t *p_;
    bool first_ = true;
public:
    explicit nibble_writer(uint8_t *p) : p_(p) {}
    void write(int value) {
        if (first_) {
            *p_ = (value << 4);
            first_ = false;
        } else {
            *p_++ |= value;
            first_ = true;
        }
    }

    uint8_t *round_off_and_get() {
        if (!first_) {
            ++p_;
        }
        return p_;
    }
};
