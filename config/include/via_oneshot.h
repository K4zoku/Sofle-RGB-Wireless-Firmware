#pragma once

#include <stdint.h>

static inline uint8_t via_oneshot_added(uint8_t requested, uint8_t owned) {
    return requested & (uint8_t)~owned;
}

static inline uint8_t via_oneshot_accumulate(uint8_t requested, uint8_t owned) {
    return requested | owned;
}
