#include <via_custom_keycode.h>

bool zmk_via_custom_keycode_to_binding(uint16_t keycode,
                                       struct zmk_behavior_binding *binding) {
    if (!binding) {
        return false;
    }

    STRUCT_SECTION_FOREACH(zmk_via_custom_keycode_provider, provider) {
        if (keycode < provider->first_keycode || keycode > provider->last_keycode ||
            !provider->to_binding || !provider->to_binding(keycode, binding)) {
            continue;
        }
        return true;
    }

    return false;
}

bool zmk_via_binding_to_custom_keycode(const struct zmk_behavior_binding *binding,
                                       uint16_t *keycode) {
    if (!binding || !keycode) {
        return false;
    }

    STRUCT_SECTION_FOREACH(zmk_via_custom_keycode_provider, provider) {
        uint16_t candidate;
        if (!provider->from_binding || !provider->from_binding(binding, &candidate) ||
            candidate < provider->first_keycode || candidate > provider->last_keycode) {
            continue;
        }
        *keycode = candidate;
        return true;
    }

    return false;
}
