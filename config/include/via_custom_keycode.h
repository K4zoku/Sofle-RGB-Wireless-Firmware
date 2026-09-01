#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/sys/iterable_sections.h>
#include <zmk/behavior.h>

struct zmk_via_custom_keycode_provider {
    uint16_t first_keycode;
    uint16_t last_keycode;
    bool (*to_binding)(uint16_t keycode, struct zmk_behavior_binding *binding);
    bool (*from_binding)(const struct zmk_behavior_binding *binding, uint16_t *keycode);
};

#define ZMK_VIA_CUSTOM_KEYCODE_PROVIDER_DEFINE(name, first, last, to, from)                        \
    static const STRUCT_SECTION_ITERABLE(zmk_via_custom_keycode_provider, name) = {                \
        .first_keycode = (first),                                                                  \
        .last_keycode = (last),                                                                    \
        .to_binding = (to),                                                                        \
        .from_binding = (from),                                                                    \
    }

bool zmk_via_custom_keycode_to_binding(uint16_t keycode,
                                       struct zmk_behavior_binding *binding);
bool zmk_via_binding_to_custom_keycode(const struct zmk_behavior_binding *binding,
                                       uint16_t *keycode);
