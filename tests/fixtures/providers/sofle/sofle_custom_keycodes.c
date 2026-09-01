#include <string.h>

#include <via_custom_keycode.h>
#include <dt-bindings/zmk/bt.h>
#include <dt-bindings/zmk/ext_power.h>
#include <dt-bindings/zmk/rgb.h>

#define SOFLE_CUSTOM_KEYCODE_BASE 0x7E00
#define SOFLE_CUSTOM_KEYCODE_LAST 0x7E0E

static bool sofle_to_binding(uint16_t keycode, struct zmk_behavior_binding *binding) {
    if (keycode < SOFLE_CUSTOM_KEYCODE_BASE || keycode > SOFLE_CUSTOM_KEYCODE_LAST) {
        return false;
    }

    switch (keycode - SOFLE_CUSTOM_KEYCODE_BASE) {
    case 0:
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = DEVICE_DT_NAME(DT_NODELABEL(bt)),
            .param1 = BT_CLR_CMD,
        };
        return true;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = DEVICE_DT_NAME(DT_NODELABEL(bt)),
            .param1 = BT_SEL_CMD,
            .param2 = keycode - SOFLE_CUSTOM_KEYCODE_BASE - 1,
        };
        return true;
    case 6:
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = DEVICE_DT_NAME(DT_NODELABEL(ext_power)),
            .param1 = EXT_POWER_TOGGLE_CMD,
        };
        return true;
    case 7:
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = DEVICE_DT_NAME(DT_NODELABEL(rgb_ug)),
            .param1 = RGB_HUD_CMD,
        };
        return true;
    case 8:
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = DEVICE_DT_NAME(DT_NODELABEL(rgb_ug)),
            .param1 = RGB_HUI_CMD,
        };
        return true;
    case 9:
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = DEVICE_DT_NAME(DT_NODELABEL(rgb_ug)),
            .param1 = RGB_SAD_CMD,
        };
        return true;
    case 10:
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = DEVICE_DT_NAME(DT_NODELABEL(rgb_ug)),
            .param1 = RGB_SAI_CMD,
        };
        return true;
    case 11:
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = DEVICE_DT_NAME(DT_NODELABEL(rgb_ug)),
            .param1 = RGB_EFF_CMD,
        };
        return true;
    case 12:
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = DEVICE_DT_NAME(DT_NODELABEL(rgb_ug)),
            .param1 = RGB_BRD_CMD,
        };
        return true;
    case 13:
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = DEVICE_DT_NAME(DT_NODELABEL(rgb_ug)),
            .param1 = RGB_BRI_CMD,
        };
        return true;
    case 14:
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = DEVICE_DT_NAME(DT_NODELABEL(rgb_ug)),
            .param1 = RGB_TOG_CMD,
        };
        return true;
    default:
        return false;
    }
}

static bool sofle_from_binding(const struct zmk_behavior_binding *binding, uint16_t *keycode) {
    if (!binding || !binding->behavior_dev) {
        return false;
    }

    if (strcmp(binding->behavior_dev, DEVICE_DT_NAME(DT_NODELABEL(bt))) == 0) {
        if (binding->param1 == BT_CLR_CMD && binding->param2 == 0) {
            *keycode = SOFLE_CUSTOM_KEYCODE_BASE;
            return true;
        }
        if (binding->param1 == BT_SEL_CMD && binding->param2 < 5) {
            *keycode = SOFLE_CUSTOM_KEYCODE_BASE + 1 + binding->param2;
            return true;
        }
    }
    if (strcmp(binding->behavior_dev, DEVICE_DT_NAME(DT_NODELABEL(ext_power))) == 0 &&
        binding->param1 == EXT_POWER_TOGGLE_CMD && binding->param2 == 0) {
        *keycode = SOFLE_CUSTOM_KEYCODE_BASE + 6;
        return true;
    }
    if (strcmp(binding->behavior_dev, DEVICE_DT_NAME(DT_NODELABEL(rgb_ug))) == 0 &&
        binding->param2 == 0) {
        switch (binding->param1) {
        case RGB_HUD_CMD: *keycode = SOFLE_CUSTOM_KEYCODE_BASE + 7; return true;
        case RGB_HUI_CMD: *keycode = SOFLE_CUSTOM_KEYCODE_BASE + 8; return true;
        case RGB_SAD_CMD: *keycode = SOFLE_CUSTOM_KEYCODE_BASE + 9; return true;
        case RGB_SAI_CMD: *keycode = SOFLE_CUSTOM_KEYCODE_BASE + 10; return true;
        case RGB_EFF_CMD: *keycode = SOFLE_CUSTOM_KEYCODE_BASE + 11; return true;
        case RGB_BRD_CMD: *keycode = SOFLE_CUSTOM_KEYCODE_BASE + 12; return true;
        case RGB_BRI_CMD: *keycode = SOFLE_CUSTOM_KEYCODE_BASE + 13; return true;
        case RGB_TOG_CMD: *keycode = SOFLE_CUSTOM_KEYCODE_BASE + 14; return true;
        default: break;
        }
    }
    return false;
}

ZMK_VIA_CUSTOM_KEYCODE_PROVIDER_DEFINE(sofle_custom_keycodes, SOFLE_CUSTOM_KEYCODE_BASE,
                                        SOFLE_CUSTOM_KEYCODE_LAST, sofle_to_binding,
                                        sofle_from_binding);
