#include <via_translation.h>

uint32_t via_encoder_tag_encode(uint8_t action, uint8_t layer_id, uint16_t payload) {
    return VIA_TRANSLATION_ENCODER_TAG | ((uint32_t)action << 24) |
           ((uint32_t)layer_id << 16) | payload;
}

bool via_encoder_tag_decode(uint32_t value, struct via_encoder_tag *out) {
    if (!out || (value & VIA_TRANSLATION_ENCODER_TAG_MASK) != VIA_TRANSLATION_ENCODER_TAG) {
        return false;
    }
    out->action = (value >> 24) & 0x0F;
    out->layer_id = (value >> 16) & 0xFF;
    out->payload = value & 0xFFFF;
    return true;
}

enum via_translation_kind via_translation_classify_keycode(uint16_t keycode) {
    if (keycode >= 0xCD && keycode <= 0xDF) return VIA_TRANSLATION_MOUSE;
    if ((keycode >= 0x04 && keycode <= 0xA4) ||
        (keycode >= 0xE0 && keycode <= 0xE7) ||
        (keycode >= 0xA5 && keycode <= 0xC2)) return VIA_TRANSLATION_BASIC;
    if (keycode >= 0x0100 && keycode <= 0x1FFF) return VIA_TRANSLATION_MODS;
    if (keycode >= 0x2000 && keycode <= 0x3FFF) return VIA_TRANSLATION_MOD_TAP;
    if (keycode >= 0x4000 && keycode <= 0x4FFF) return VIA_TRANSLATION_LAYER_TAP;
    if (keycode >= 0x5000 && keycode <= 0x51FF) return VIA_TRANSLATION_LAYER_MOD;
    if (keycode >= 0x5200 && keycode <= 0x521F) return VIA_TRANSLATION_TO;
    if (keycode >= 0x5220 && keycode <= 0x523F) return VIA_TRANSLATION_MOMENTARY;
    if (keycode >= 0x5240 && keycode <= 0x525F) return VIA_TRANSLATION_DEFAULT;
    if (keycode >= 0x5260 && keycode <= 0x527F) return VIA_TRANSLATION_TOGGLE;
    if (keycode >= 0x5280 && keycode <= 0x529F) return VIA_TRANSLATION_ONE_SHOT_LAYER;
    if (keycode >= 0x52A0 && keycode <= 0x52BF) return VIA_TRANSLATION_ONE_SHOT_MOD;
    if (keycode >= 0x52C0 && keycode <= 0x52DF) return VIA_TRANSLATION_TAP_TOGGLE;
    if (keycode >= 0x52E0 && keycode <= 0x52FF) return VIA_TRANSLATION_PERSISTENT_DEFAULT;
    if (keycode >= 0x7700 && keycode <= 0x777F) return VIA_TRANSLATION_BASIC;
    return VIA_TRANSLATION_UNSUPPORTED;
}
