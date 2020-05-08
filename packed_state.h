#pragma once

struct PackedState {
    uint8_t data_[32] = {};

    friend bool operator==(const PackedState& a, const PackedState& b) {
        return memcmp(a.data_, b.data_, 32) == 0;
    }
    friend bool operator!=(const PackedState& a, const PackedState& b) {
        return memcmp(a.data_, b.data_, 32) != 0;
    }
    friend bool operator<(const PackedState& a, const PackedState& b) {
        return memcmp(a.data_, b.data_, 32) < 0;
    }
    friend bool operator>(const PackedState& a, const PackedState& b) {
        return memcmp(a.data_, b.data_, 32) > 0;
    }
    friend bool operator<=(const PackedState& a, const PackedState& b) {
        return memcmp(a.data_, b.data_, 32) <= 0;
    }
    friend bool operator>=(const PackedState& a, const PackedState& b) {
        return memcmp(a.data_, b.data_, 32) >= 0;
    }
};
