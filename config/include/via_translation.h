#pragma once

#include <stdbool.h>
#include <stdint.h>

#define VIA_TRANSLATION_ENCODER_TAG 0xA0000000u
#define VIA_TRANSLATION_ENCODER_TAG_MASK 0xF0000000u

enum via_translation_kind {
    VIA_TRANSLATION_BASIC,
    VIA_TRANSLATION_MODS,
    VIA_TRANSLATION_MOD_TAP,
    VIA_TRANSLATION_LAYER_TAP,
    VIA_TRANSLATION_LAYER_MOD,
    VIA_TRANSLATION_MOMENTARY,
    VIA_TRANSLATION_TO,
    VIA_TRANSLATION_TOGGLE,
    VIA_TRANSLATION_DEFAULT,
    VIA_TRANSLATION_PERSISTENT_DEFAULT,
    VIA_TRANSLATION_ONE_SHOT_LAYER,
    VIA_TRANSLATION_ONE_SHOT_MOD,
    VIA_TRANSLATION_TAP_TOGGLE,
    VIA_TRANSLATION_MOUSE,
    VIA_TRANSLATION_UNSUPPORTED,
};

struct via_encoder_tag {
    uint8_t action;
    uint8_t layer_id;
    uint16_t payload;
};

uint32_t via_encoder_tag_encode(uint8_t action, uint8_t layer_id, uint16_t payload);
bool via_encoder_tag_decode(uint32_t value, struct via_encoder_tag *out);
enum via_translation_kind via_translation_classify_keycode(uint16_t keycode);
