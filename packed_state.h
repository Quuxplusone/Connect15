#pragma once

#include <functional>
#include <string>
#include <string.h>

struct PackedState {
    uint8_t data_[32] = {};

    PackedState() = default;

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

template<>
struct std::hash<PackedState> {
    size_t operator()(const PackedState& x) const {
#if defined(_LIBCPP_VERSION)
        return std::__do_string_hash(x.data_, x.data_ + 32);
#elif defined(__GLIBCXX__)
        return std::_Hash_bytes(x.data_, 32, 0xC70F6907uL);
#endif
    }
};
