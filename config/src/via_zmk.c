#include <zephyr/devicetree.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <dt-bindings/zmk/hid_usage_pages.h>
#include <drivers/behavior.h>
#include <dt-bindings/zmk/modifiers.h>
#include <raw_hid/events.h>
#include <zmk/behavior.h>
#include <zmk/keymap.h>
#include <zmk/physical_layouts.h>
#include <zmk/matrix.h>
#include <zmk/sensors.h>
#include <zmk/rgb_underglow.h>
#include <dt-bindings/zmk/bt.h>
#include <dt-bindings/zmk/ext_power.h>
#include <dt-bindings/zmk/rgb.h>

LOG_MODULE_REGISTER(zmk_via, CONFIG_ZMK_LOG_LEVEL);

#define VIA_PROTOCOL_VERSION 0x000D
#define VIA_REPORT_SIZE CONFIG_RAW_HID_REPORT_SIZE
#define VIA_MATRIX_NODE DT_NODELABEL(via_matrix)
#define VIA_KEYMAP_ROWS DT_PROP(VIA_MATRIX_NODE, rows)
#define VIA_KEYMAP_COLS DT_PROP(VIA_MATRIX_NODE, columns)
#define VIA_KEYMAP_SLOTS (VIA_KEYMAP_ROWS * VIA_KEYMAP_COLS)

#define VIA_CMD_GET_PROTOCOL_VERSION 0x01
#define VIA_CMD_GET_KEYBOARD_VALUE 0x02
#define VIA_CMD_SET_KEYBOARD_VALUE 0x03
#define VIA_CMD_GET_KEYCODE 0x04
#define VIA_CMD_SET_KEYCODE 0x05
#define VIA_CMD_RESET 0x06
#define VIA_CMD_MACRO_GET_COUNT 0x0C
#define VIA_CMD_MACRO_GET_BUFFER_SIZE 0x0D
#define VIA_CMD_MACRO_GET_BUFFER 0x0E
#define VIA_CMD_MACRO_SET_BUFFER 0x0F
#define VIA_CMD_MACRO_RESET 0x10
#define VIA_CMD_GET_LAYER_COUNT 0x11
#define VIA_CMD_GET_BUFFER 0x12
#define VIA_CMD_GET_ENCODER 0x14
#define VIA_CMD_SET_ENCODER 0x15
#define VIA_CMD_SET_BUFFER 0x13
#define VIA_CMD_UNHANDLED 0xFF
#define VIA_CMD_CUSTOM_SET_VALUE 0x07
#define VIA_CMD_CUSTOM_GET_VALUE 0x08
#define VIA_CMD_CUSTOM_SAVE 0x09
#define VIA_RGBLIGHT_CHANNEL 0x02
#define VIA_RGB_BRIGHTNESS 0x01
#define VIA_RGB_EFFECT 0x02
#define VIA_RGB_EFFECT_SPEED 0x03
#define VIA_RGB_COLOR 0x04
#define VIA_RGB_EFFECT_COUNT 4

#define VIA_VALUE_UPTIME 0x01
#define VIA_VALUE_LAYOUT_OPTIONS 0x02
#define VIA_VALUE_SWITCH_MATRIX_STATE 0x03
#define VIA_VALUE_FIRMWARE_VERSION 0x04
#define VIA_VALUE_DEVICE_INDICATION 0x05
#define VIA_VALUE_KEYCODES_VERSION 0x06

#define QMK_KC_NO 0x0000
#define QMK_KC_TRANSPARENT 0x0001
#define QMK_QK_MODS 0x0100
#define QMK_QK_MODS_MAX 0x1FFF
#define QMK_QK_MOD_TAP 0x2000
#define QMK_QK_MOD_TAP_MAX 0x3FFF
#define QMK_QK_LAYER_TAP 0x4000
#define QMK_QK_LAYER_TAP_MAX 0x4FFF
#define QMK_QK_TO 0x5200
#define QMK_QK_TO_MAX 0x521F
#define QMK_QK_MOMENTARY 0x5220
#define QMK_QK_MOMENTARY_MAX 0x523F
#define QMK_QK_TOGGLE_LAYER 0x5260
#define QMK_QK_TOGGLE_LAYER_MAX 0x527F
#define QMK_QK_LAYER_MOD 0x5000
#define QMK_QK_LAYER_MOD_MAX 0x51FF
#define VIA_CUSTOM_KEYCODE_BASE 0x7E00
#define VIA_CUSTOM_KEYCODE_LAST 0x7E0E

#if DT_NODE_EXISTS(DT_NODELABEL(kp))
#define VIA_KP_BEHAVIOR DEVICE_DT_NAME(DT_NODELABEL(kp))
#else
#define VIA_KP_BEHAVIOR ""
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(mt))
#define VIA_MT_BEHAVIOR DEVICE_DT_NAME(DT_NODELABEL(mt))
#else
#define VIA_MT_BEHAVIOR ""
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(lt))
#define VIA_LT_BEHAVIOR DEVICE_DT_NAME(DT_NODELABEL(lt))
#else
#define VIA_LT_BEHAVIOR ""
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(mo))
#define VIA_MO_BEHAVIOR DEVICE_DT_NAME(DT_NODELABEL(mo))
#else
#define VIA_MO_BEHAVIOR ""
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(to))
#define VIA_TO_BEHAVIOR DEVICE_DT_NAME(DT_NODELABEL(to))
#else
#define VIA_TO_BEHAVIOR ""
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(tog))
#define VIA_TOG_BEHAVIOR DEVICE_DT_NAME(DT_NODELABEL(tog))
#else
#define VIA_TOG_BEHAVIOR ""
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(inc_dec_kp))
#define VIA_INC_DEC_KP_BEHAVIOR DEVICE_DT_NAME(DT_NODELABEL(inc_dec_kp))
#else
#define VIA_INC_DEC_KP_BEHAVIOR ""
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(trans))
#define VIA_TRANS_BEHAVIOR DEVICE_DT_NAME(DT_NODELABEL(trans))
#else
#define VIA_TRANS_BEHAVIOR ""
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(none))
#define VIA_NONE_BEHAVIOR DEVICE_DT_NAME(DT_NODELABEL(none))
#else
#define VIA_NONE_BEHAVIOR ""
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(bt))
#define VIA_BT_BEHAVIOR DEVICE_DT_NAME(DT_NODELABEL(bt))
#else
#define VIA_BT_BEHAVIOR ""
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(ext_power))
#define VIA_EXT_POWER_BEHAVIOR DEVICE_DT_NAME(DT_NODELABEL(ext_power))
#else
#define VIA_EXT_POWER_BEHAVIOR ""
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(rgb_ug))
#define VIA_RGB_UG_BEHAVIOR DEVICE_DT_NAME(DT_NODELABEL(rgb_ug))
#else
#define VIA_RGB_UG_BEHAVIOR ""
#endif

BUILD_ASSERT(VIA_REPORT_SIZE == 32, "Stock VIA requires 32-byte Raw HID reports");

/* The keyboard supplies this virtual-matrix-to-ZMK-position adapter in Devicetree. */
static const uint8_t via_position_map[] =
    DT_PROP(DT_NODELABEL(via_matrix), map);
BUILD_ASSERT(DT_PROP_LEN(DT_NODELABEL(via_matrix), map) == VIA_KEYMAP_SLOTS,
             "VIA map length must equal rows * columns");
BUILD_ASSERT(ZMK_KEYMAP_LEN <= UINT8_MAX, "VIA map indices require an 8-bit ZMK keymap");
static uint8_t via_qmk_mods_to_zmk(uint8_t qmk_mods) {
    const bool right = (qmk_mods & BIT(4)) != 0;
    const uint8_t types = qmk_mods & 0x0F;
    uint8_t zmk_mods = 0;

    if (types & BIT(0)) {
        zmk_mods |= right ? MOD_RCTL : MOD_LCTL;
    }
    if (types & BIT(1)) {
        zmk_mods |= right ? MOD_RSFT : MOD_LSFT;
    }
    if (types & BIT(2)) {
        zmk_mods |= right ? MOD_RALT : MOD_LALT;
    }
    if (types & BIT(3)) {
        zmk_mods |= right ? MOD_RGUI : MOD_LGUI;
    }

    return zmk_mods;
}

static bool via_zmk_mods_to_qmk(uint8_t zmk_mods, uint8_t *qmk_mods) {
    const uint8_t left = zmk_mods & 0x0F;
    const uint8_t right = (zmk_mods >> 4) & 0x0F;

    if (left && right) {
        return false;
    }

    *qmk_mods = left ? left : (right ? (right | BIT(4)) : 0);
    return true;
}
static bool via_layer_index_to_id(uint8_t index, zmk_keymap_layer_id_t *id) {
    if (index >= ZMK_KEYMAP_LAYERS_LEN) {
        return false;
    }
    *id = zmk_keymap_layer_index_to_id(index);
    return *id != ZMK_KEYMAP_LAYER_ID_INVAL;
}

static bool via_layer_id_to_index(zmk_keymap_layer_id_t id, uint8_t *index) {
    for (uint8_t candidate = 0; candidate < ZMK_KEYMAP_LAYERS_LEN; candidate++) {
        if (zmk_keymap_layer_index_to_id(candidate) == id) {
            *index = candidate;
            return true;
        }
    }
    return false;
}

static bool via_qmk_basic_to_usage(uint8_t keycode, uint32_t *usage) {
    switch (keycode) {
    case 0xA5: /* KC_SYSTEM_POWER */
        *usage = ZMK_HID_USAGE(HID_USAGE_GD, 0x81);
        return true;
    case 0xA6: /* KC_SYSTEM_SLEEP */
        *usage = ZMK_HID_USAGE(HID_USAGE_GD, 0x82);
        return true;
    case 0xA7: /* KC_SYSTEM_WAKE */
        *usage = ZMK_HID_USAGE(HID_USAGE_GD, 0x83);
        return true;
    case 0xA8: /* KC_AUDIO_MUTE */
        *usage = ZMK_HID_USAGE(HID_USAGE_CONSUMER, 0xE2);
        return true;
    case 0xA9: /* KC_AUDIO_VOL_UP */
        *usage = ZMK_HID_USAGE(HID_USAGE_CONSUMER, 0xE9);
        return true;
    case 0xAA: /* KC_AUDIO_VOL_DOWN */
        *usage = ZMK_HID_USAGE(HID_USAGE_CONSUMER, 0xEA);
        return true;
    case 0xAB: /* KC_MEDIA_NEXT_TRACK */
        *usage = ZMK_HID_USAGE(HID_USAGE_CONSUMER, 0xB5);
        return true;
    case 0xAC: /* KC_MEDIA_PREV_TRACK */
        *usage = ZMK_HID_USAGE(HID_USAGE_CONSUMER, 0xB6);
        return true;
    case 0xAD: /* KC_MEDIA_STOP */
        *usage = ZMK_HID_USAGE(HID_USAGE_CONSUMER, 0xB7);
        return true;
    case 0xAE: /* KC_MEDIA_PLAY_PAUSE */
        *usage = ZMK_HID_USAGE(HID_USAGE_CONSUMER, 0xCD);
        return true;
    default:
        break;
    }

    /* The QMK basic range uses the USB keyboard page for these usages. */
    if ((keycode >= 0x04 && keycode <= 0xA4) || (keycode >= 0xE0 && keycode <= 0xE7)) {
        *usage = ZMK_HID_USAGE(HID_USAGE_KEY, keycode);
        return true;
    }

    return false;
}

static bool via_usage_to_qmk_basic(uint32_t usage, uint8_t *keycode) {
    const uint8_t page = ZMK_HID_USAGE_PAGE(usage);
    const uint16_t id = ZMK_HID_USAGE_ID(usage);

    if (page == HID_USAGE_KEY && id <= 0xFF) {
        if ((id >= 0x04 && id <= 0xA4) || (id >= 0xE0 && id <= 0xE7)) {
            *keycode = id;
            return true;
        }
    }

    if (page == HID_USAGE_GD) {
        switch (id) {
        case 0x81:
            *keycode = 0xA5;
            return true;
        case 0x82:
            *keycode = 0xA6;
            return true;
        case 0x83:
            *keycode = 0xA7;
            return true;
        default:
            break;
        }
    }

    if (page == HID_USAGE_CONSUMER) {
        switch (id) {
        case 0xE2:
            *keycode = 0xA8;
            return true;
        case 0xE9:
            *keycode = 0xA9;
            return true;
        case 0xEA:
            *keycode = 0xAA;
            return true;
        case 0xB5:
            *keycode = 0xAB;
            return true;
        case 0xB6:
            *keycode = 0xAC;
            return true;
        case 0xB7:
            *keycode = 0xAD;
            return true;
        case 0xCD:
            *keycode = 0xAE;
            return true;
        default:
            break;
        }
    }

    return false;
}

static bool via_custom_keycode_to_binding(uint16_t keycode,
                                          struct zmk_behavior_binding *binding) {
#if IS_ENABLED(CONFIG_ZMK_VIA_CUSTOM_KEYCODES)
    if (keycode < VIA_CUSTOM_KEYCODE_BASE || keycode > VIA_CUSTOM_KEYCODE_LAST) {
        return false;
    }

    switch (keycode - VIA_CUSTOM_KEYCODE_BASE) {
    case 0:
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = VIA_BT_BEHAVIOR,
            .param1 = BT_CLR_CMD,
        };
        return true;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = VIA_BT_BEHAVIOR,
            .param1 = BT_SEL_CMD,
            .param2 = keycode - VIA_CUSTOM_KEYCODE_BASE - 1,
        };
        return true;
    case 6:
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = VIA_EXT_POWER_BEHAVIOR,
            .param1 = EXT_POWER_TOGGLE_CMD,
        };
        return true;
    case 7:
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = VIA_RGB_UG_BEHAVIOR,
            .param1 = RGB_HUD_CMD,
        };
        return true;
    case 8:
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = VIA_RGB_UG_BEHAVIOR,
            .param1 = RGB_HUI_CMD,
        };
        return true;
    case 9:
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = VIA_RGB_UG_BEHAVIOR,
            .param1 = RGB_SAD_CMD,
        };
        return true;
    case 10:
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = VIA_RGB_UG_BEHAVIOR,
            .param1 = RGB_SAI_CMD,
        };
        return true;
    case 11:
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = VIA_RGB_UG_BEHAVIOR,
            .param1 = RGB_EFF_CMD,
        };
        return true;
    case 12:
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = VIA_RGB_UG_BEHAVIOR,
            .param1 = RGB_BRD_CMD,
        };
        return true;
    case 13:
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = VIA_RGB_UG_BEHAVIOR,
            .param1 = RGB_BRI_CMD,
        };
        return true;
    case 14:
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = VIA_RGB_UG_BEHAVIOR,
            .param1 = RGB_TOG_CMD,
        };
        return true;
    default:
        return false;
    }
#else
    (void)keycode;
    (void)binding;
    return false;
#endif
}

static bool via_qmk_to_binding(uint16_t keycode, struct zmk_behavior_binding *binding) {
    uint32_t usage;

    if (keycode == QMK_KC_NO) {
        *binding = (struct zmk_behavior_binding){.behavior_dev = VIA_NONE_BEHAVIOR};
        return true;
    }
    if (keycode == QMK_KC_TRANSPARENT) {
        *binding = (struct zmk_behavior_binding){.behavior_dev = VIA_TRANS_BEHAVIOR};
        return true;
    }
    if (via_custom_keycode_to_binding(keycode, binding)) {
        return true;
    }

    if (keycode >= QMK_QK_MODS && keycode <= QMK_QK_MODS_MAX) {
        const uint8_t qmk_mods = (keycode >> 8) & 0x1F;
        if (!via_qmk_basic_to_usage(keycode & 0xFF, &usage)) {
            return false;
        }
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = VIA_KP_BEHAVIOR,
            .param1 = ((uint32_t)via_qmk_mods_to_zmk(qmk_mods) << 24) | usage,
        };
        return true;
    }

    if (keycode >= QMK_QK_MOD_TAP && keycode <= QMK_QK_MOD_TAP_MAX) {
        if (!via_qmk_basic_to_usage(keycode & 0xFF, &usage)) {
            return false;
        }
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = VIA_MT_BEHAVIOR,
            .param1 = via_qmk_mods_to_zmk((keycode >> 8) & 0x1F),
            .param2 = usage,
        };
        return true;
    }

    if (keycode >= QMK_QK_LAYER_TAP && keycode <= QMK_QK_LAYER_TAP_MAX) {
        zmk_keymap_layer_id_t layer_id;
        if (!via_layer_index_to_id((keycode >> 8) & 0x0F, &layer_id) ||
            !via_qmk_basic_to_usage(keycode & 0xFF, &usage)) {
            return false;
        }
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = VIA_LT_BEHAVIOR,
            .param1 = layer_id,
            .param2 = usage,
        };
        return true;
    }

    if (keycode >= QMK_QK_TO && keycode <= QMK_QK_TO_MAX) {
        zmk_keymap_layer_id_t layer_id;
        if (!via_layer_index_to_id(keycode & 0x1F, &layer_id)) {
            return false;
        }
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = VIA_TO_BEHAVIOR,
            .param1 = layer_id,
        };
        return true;
    }
    if (keycode >= QMK_QK_MOMENTARY && keycode <= QMK_QK_MOMENTARY_MAX) {
        zmk_keymap_layer_id_t layer_id;
        if (!via_layer_index_to_id(keycode & 0x1F, &layer_id)) {
            return false;
        }
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = VIA_MO_BEHAVIOR,
            .param1 = layer_id,
        };
        return true;
    }
    if (keycode >= QMK_QK_TOGGLE_LAYER && keycode <= QMK_QK_TOGGLE_LAYER_MAX) {
        zmk_keymap_layer_id_t layer_id;
        if (!via_layer_index_to_id(keycode & 0x1F, &layer_id)) {
            return false;
        }
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = VIA_TOG_BEHAVIOR,
            .param1 = layer_id,
        };
        return true;
    }

    /* QMK LM/DF/PDF/OSL and custom ranges have no lossless ZMK equivalent. */
    if (!via_qmk_basic_to_usage(keycode, &usage)) {
        return false;
    }
    *binding = (struct zmk_behavior_binding){
        .behavior_dev = VIA_KP_BEHAVIOR,
        .param1 = usage,
    };
    return true;
}

static bool via_binding_to_custom_keycode(const struct zmk_behavior_binding *binding,
                                          uint16_t *keycode) {
#if IS_ENABLED(CONFIG_ZMK_VIA_CUSTOM_KEYCODES)
    if (!binding || !binding->behavior_dev) {
        return false;
    }

    if (strcmp(binding->behavior_dev, VIA_BT_BEHAVIOR) == 0) {
        if (binding->param1 == BT_CLR_CMD && binding->param2 == 0) {
            *keycode = VIA_CUSTOM_KEYCODE_BASE;
            return true;
        }
        if (binding->param1 == BT_SEL_CMD && binding->param2 < 5) {
            *keycode = VIA_CUSTOM_KEYCODE_BASE + 1 + binding->param2;
            return true;
        }
    }
    if (strcmp(binding->behavior_dev, VIA_EXT_POWER_BEHAVIOR) == 0 &&
        binding->param1 == EXT_POWER_TOGGLE_CMD && binding->param2 == 0) {
        *keycode = VIA_CUSTOM_KEYCODE_BASE + 6;
        return true;
    }
    if (strcmp(binding->behavior_dev, VIA_RGB_UG_BEHAVIOR) == 0 && binding->param2 == 0) {
        switch (binding->param1) {
        case RGB_HUD_CMD:
            *keycode = VIA_CUSTOM_KEYCODE_BASE + 7;
            return true;
        case RGB_HUI_CMD:
            *keycode = VIA_CUSTOM_KEYCODE_BASE + 8;
            return true;
        case RGB_SAD_CMD:
            *keycode = VIA_CUSTOM_KEYCODE_BASE + 9;
            return true;
        case RGB_SAI_CMD:
            *keycode = VIA_CUSTOM_KEYCODE_BASE + 10;
            return true;
        case RGB_EFF_CMD:
            *keycode = VIA_CUSTOM_KEYCODE_BASE + 11;
            return true;
        case RGB_BRD_CMD:
            *keycode = VIA_CUSTOM_KEYCODE_BASE + 12;
            return true;
        case RGB_BRI_CMD:
            *keycode = VIA_CUSTOM_KEYCODE_BASE + 13;
            return true;
        case RGB_TOG_CMD:
            *keycode = VIA_CUSTOM_KEYCODE_BASE + 14;
            return true;
        default:
            break;
        }
    }
    return false;
#else
    (void)binding;
    (void)keycode;
    return false;
#endif
}

static bool via_binding_to_qmk(const struct zmk_behavior_binding *binding, uint16_t *keycode) {
    uint8_t basic;
    uint8_t qmk_mods;

    if (!binding || !binding->behavior_dev) {
        return false;
    }
    if (strcmp(binding->behavior_dev, VIA_NONE_BEHAVIOR) == 0) {
        *keycode = QMK_KC_NO;
        return true;
    }
    if (strcmp(binding->behavior_dev, VIA_TRANS_BEHAVIOR) == 0) {
        *keycode = QMK_KC_TRANSPARENT;
        return true;
    }
    if (via_binding_to_custom_keycode(binding, keycode)) {
        return true;
    }
    if (strcmp(binding->behavior_dev, VIA_KP_BEHAVIOR) == 0 &&
        via_usage_to_qmk_basic(binding->param1, &basic)) {
        if (!via_zmk_mods_to_qmk(binding->param1 >> 24, &qmk_mods)) {
            return false;
        }
        *keycode = ((uint16_t)qmk_mods << 8) | basic;
        return true;
    }
    if (strcmp(binding->behavior_dev, VIA_MT_BEHAVIOR) == 0 &&
        via_usage_to_qmk_basic(binding->param2, &basic) &&
        via_zmk_mods_to_qmk(binding->param1, &qmk_mods)) {
        *keycode = QMK_QK_MOD_TAP | ((uint16_t)qmk_mods << 8) | basic;
        return true;
    }
    if (strcmp(binding->behavior_dev, VIA_LT_BEHAVIOR) == 0) {
        uint8_t layer_index;
        if (via_layer_id_to_index(binding->param1, &layer_index) &&
            layer_index < 16 && via_usage_to_qmk_basic(binding->param2, &basic)) {
            *keycode = QMK_QK_LAYER_TAP | ((uint16_t)layer_index << 8) | basic;
            return true;
        }
    }
    if (strcmp(binding->behavior_dev, VIA_TO_BEHAVIOR) == 0) {
        uint8_t layer_index;
        if (via_layer_id_to_index(binding->param1, &layer_index) && layer_index < 32) {
            *keycode = QMK_QK_TO | layer_index;
            return true;
        }
    }
    if (strcmp(binding->behavior_dev, VIA_MO_BEHAVIOR) == 0) {
        uint8_t layer_index;
        if (via_layer_id_to_index(binding->param1, &layer_index) && layer_index < 32) {
            *keycode = QMK_QK_MOMENTARY | layer_index;
            return true;
        }
    }
    if (strcmp(binding->behavior_dev, VIA_TOG_BEHAVIOR) == 0) {
        uint8_t layer_index;
        if (via_layer_id_to_index(binding->param1, &layer_index) && layer_index < 32) {
            *keycode = QMK_QK_TOGGLE_LAYER | layer_index;
            return true;
        }
    }

    return false;
}

static bool via_read_encoder(uint8_t layer, uint8_t encoder, bool clockwise,
                             uint16_t *keycode) {
    if (layer >= ZMK_KEYMAP_LAYERS_LEN || encoder >= ZMK_KEYMAP_SENSORS_LEN) {
        return false;
    }

    const zmk_keymap_layer_id_t layer_id = zmk_keymap_layer_index_to_id(layer);
    const struct zmk_behavior_binding *binding =
        zmk_keymap_get_sensor_binding_at_idx(layer_id, encoder);
    if (!binding || strcmp(binding->behavior_dev, VIA_INC_DEC_KP_BEHAVIOR) != 0) {
        *keycode = QMK_KC_NO;
        return true;
    }

    const uint32_t usage = clockwise ? binding->param1 : binding->param2;
    uint8_t basic;
    uint8_t qmk_mods;
    if (!via_usage_to_qmk_basic(usage, &basic) ||
        !via_zmk_mods_to_qmk(usage >> 24, &qmk_mods)) {
        *keycode = QMK_KC_NO;
        return true;
    }

    *keycode = ((uint16_t)qmk_mods << 8) | basic;
    return true;
}

static bool via_slot_to_zmk_position(uint8_t row, uint8_t column, uint8_t *position) {
    if (row >= VIA_KEYMAP_ROWS || column >= VIA_KEYMAP_COLS) {
        return false;
    }

    *position = via_position_map[row * VIA_KEYMAP_COLS + column];
    const uint32_t *selected_to_stock;
    const int selected_length =
        zmk_physical_layouts_get_selected_to_stock_position_map(&selected_to_stock);
    return selected_length > 0 && *position != UINT8_MAX && *position < selected_length &&
           selected_to_stock[*position] < ZMK_KEYMAP_LEN;
}

static bool via_read_keycode(uint8_t layer, uint8_t row, uint8_t column, uint16_t *keycode) {
    uint8_t position;
    if (layer >= ZMK_KEYMAP_LAYERS_LEN || !via_slot_to_zmk_position(row, column, &position)) {
        return false;
    }

    const zmk_keymap_layer_id_t layer_id = zmk_keymap_layer_index_to_id(layer);
    const struct zmk_behavior_binding *binding =
        zmk_keymap_get_layer_binding_at_idx(layer_id, position);
    if (!via_binding_to_qmk(binding, keycode)) {
        *keycode = QMK_KC_NO;
    }
    return true;
}
static bool via_binding_is_unsupported(uint8_t layer, uint8_t position) {
    if (layer >= ZMK_KEYMAP_LAYERS_LEN) {
        return false;
    }
    const zmk_keymap_layer_id_t layer_id = zmk_keymap_layer_index_to_id(layer);
    const struct zmk_behavior_binding *binding =
        zmk_keymap_get_layer_binding_at_idx(layer_id, position);
    uint16_t keycode;
    return binding && !via_binding_to_qmk(binding, &keycode);
}

static bool via_make_binding(uint16_t keycode, struct zmk_behavior_binding *binding) {
    if (!via_qmk_to_binding(keycode, binding)) {
        return false;
    }
    if (zmk_behavior_validate_binding(binding) >= 0) {
        return true;
    }
    /* ext_power has no metadata provider in ZMK 0.3, but this fixed custom
     * binding is already part of the stock Sofle keymap. */
    return strcmp(binding->behavior_dev, VIA_EXT_POWER_BEHAVIOR) == 0 &&
           binding->param1 == EXT_POWER_TOGGLE_CMD && binding->param2 == 0;
}

static bool via_write_keycode(uint8_t layer, uint8_t row, uint8_t column, uint16_t keycode) {
    uint8_t position;
    struct zmk_behavior_binding binding;

    if (layer >= ZMK_KEYMAP_LAYERS_LEN || !via_slot_to_zmk_position(row, column, &position)) {
        return false;
    }
    if (keycode == QMK_KC_NO && via_binding_is_unsupported(layer, position)) {
        /* VIA uses KC_NO for bindings it cannot represent; do not erase ZMK-only behavior. */
        return true;
    }
    if (!via_make_binding(keycode, &binding)) {
        return false;
    }

    const zmk_keymap_layer_id_t layer_id = zmk_keymap_layer_index_to_id(layer);
    return zmk_keymap_set_layer_binding_at_idx(layer_id, position, binding) >= 0;

}
static size_t via_keymap_size(void) {
    return (size_t)ZMK_KEYMAP_LAYERS_LEN * VIA_KEYMAP_SLOTS * sizeof(uint16_t);
}

static bool via_get_buffer(uint16_t offset, uint8_t size, uint8_t *data) {
    if (size > 28 || (size_t)offset + size > via_keymap_size()) {
        return false;
    }

    for (uint8_t i = 0; i < size; i++) {
        const size_t byte_offset = (size_t)offset + i;
        const size_t word = byte_offset / 2;
        const uint8_t layer = word / VIA_KEYMAP_SLOTS;
        const uint8_t slot = word % VIA_KEYMAP_SLOTS;
        uint16_t keycode = QMK_KC_NO;
        (void)via_read_keycode(layer, slot / VIA_KEYMAP_COLS, slot % VIA_KEYMAP_COLS, &keycode);
        data[i] = (byte_offset & 1) ? (keycode & 0xFF) : (keycode >> 8);
    }
    return true;
}

static bool via_set_buffer(uint16_t offset, uint8_t size, const uint8_t *data) {
    struct zmk_behavior_binding bindings[14];
    zmk_keymap_layer_id_t layer_ids[14];
    uint8_t layers[14];
    uint8_t positions[14];
    const uint8_t count = size / 2;

    if ((offset & 1) || (size & 1) || size > 28 || (size_t)offset + size > via_keymap_size()) {
        return false;
    }

    for (uint8_t i = 0; i < count; i++) {
        const size_t word = ((size_t)offset / 2) + i;
        const uint8_t layer = word / VIA_KEYMAP_SLOTS;
        const uint8_t slot = word % VIA_KEYMAP_SLOTS;
        const uint16_t keycode = ((uint16_t)data[i * 2] << 8) | data[i * 2 + 1];

        if (!via_layer_index_to_id(layer, &layer_ids[i])) {
            return false;
        }

        if (!via_slot_to_zmk_position(slot / VIA_KEYMAP_COLS, slot % VIA_KEYMAP_COLS,
                                      &positions[i])) {
            /* Unused virtual slots are represented as KC_NO in the bulk buffer. */
            if (keycode != QMK_KC_NO) {
                return false;
            }
            layers[i] = layer;
            positions[i] = UINT8_MAX;
            continue;
        }
        layers[i] = layer;
        if (keycode == QMK_KC_NO && via_binding_is_unsupported(layer, positions[i])) {
            positions[i] = UINT8_MAX;
            continue;
        }
        if (!via_make_binding(keycode, &bindings[i])) {
            return false;
        }
    }

    for (uint8_t i = 0; i < count; i++) {
        if (positions[i] == UINT8_MAX) {
            continue;
        }
        if (zmk_keymap_set_layer_binding_at_idx(layer_ids[i], positions[i], bindings[i]) < 0) {
            return false;
        }
    }
    return true;
}

static void via_write_u32(uint8_t *data, uint32_t value) {
    data[0] = value >> 24;
    data[1] = value >> 16;
    data[2] = value >> 8;
    data[3] = value;
}

#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW)
static bool via_rgb_get_value(uint8_t *report) {
    if (report[1] != VIA_RGBLIGHT_CHANNEL) {
        return false;
    }

    struct zmk_led_hsb color;
    bool on;
    if (zmk_rgb_underglow_get_state(&on) < 0 ||
        zmk_rgb_underglow_get_hsb(&color) < 0) {
        return false;
    }

    switch (report[2]) {
    case VIA_RGB_BRIGHTNESS:
        report[3] = ((uint16_t)color.b * UINT8_MAX) / 100;
        return true;
    case VIA_RGB_EFFECT: {
        const int effect = zmk_rgb_underglow_get_effect();
        report[3] = on && effect >= 0 && effect < VIA_RGB_EFFECT_COUNT ? effect + 1 : 0;
        return true;
    }
    case VIA_RGB_EFFECT_SPEED:
        report[3] = ((uint16_t)zmk_rgb_underglow_get_speed() * UINT8_MAX) / 5;
        return true;
    case VIA_RGB_COLOR:
        report[3] = ((uint32_t)color.h * UINT8_MAX) / 360;
        report[4] = ((uint16_t)color.s * UINT8_MAX) / 100;
        return true;
    default:
        return false;
    }
}

static bool via_rgb_set_value(uint8_t *report) {
    if (report[1] != VIA_RGBLIGHT_CHANNEL) {
        return false;
    }

    struct zmk_led_hsb color;
    if (zmk_rgb_underglow_get_hsb(&color) < 0) {
        return false;
    }

    switch (report[2]) {
    case VIA_RGB_BRIGHTNESS:
        color.b = ((uint16_t)report[3] * 100 + 127) / UINT8_MAX;
        return zmk_rgb_underglow_set_hsb(color) >= 0;
    case VIA_RGB_EFFECT:
        if (report[3] == 0) {
            return zmk_rgb_underglow_off() >= 0;
        }
        return report[3] <= VIA_RGB_EFFECT_COUNT &&
               zmk_rgb_underglow_on() >= 0 &&
               zmk_rgb_underglow_select_effect(report[3] - 1) >= 0;
    case VIA_RGB_EFFECT_SPEED: {
        const uint8_t speed = MAX(1, MIN(5, ((uint16_t)report[3] * 5 + 127) / UINT8_MAX));
        return zmk_rgb_underglow_set_speed(speed) >= 0;
    }
    case VIA_RGB_COLOR:
        color.h = ((uint32_t)report[3] * 360 + 127) / UINT8_MAX;
        color.s = ((uint16_t)report[4] * 100 + 127) / UINT8_MAX;
        return zmk_rgb_underglow_set_hsb(color) >= 0;
    default:
        return false;
    }
}
#endif

static bool via_handle_report(uint8_t *report, bool *changed_out) {
    bool changed = false;
    bool handled = true;
    const uint8_t command = report[0];

    switch (command) {
    case VIA_CMD_GET_PROTOCOL_VERSION:
        report[1] = VIA_PROTOCOL_VERSION >> 8;
        report[2] = VIA_PROTOCOL_VERSION;
        break;
    case VIA_CMD_GET_KEYBOARD_VALUE:
        switch (report[1]) {
        case VIA_VALUE_UPTIME:
            via_write_u32(&report[2], k_uptime_get_32());
            break;
        case VIA_VALUE_LAYOUT_OPTIONS:
            via_write_u32(&report[2], 0);
            break;
        case VIA_VALUE_FIRMWARE_VERSION:
            via_write_u32(&report[2], 0x00000001);
            break;
        case VIA_VALUE_KEYCODES_VERSION:
            via_write_u32(&report[2], 0x00000008);
            break;
        case VIA_VALUE_SWITCH_MATRIX_STATE:
            /* The virtual matrix has no electrical scan state. */
            memset(&report[2], 0, MIN(28, VIA_REPORT_SIZE - 2));
            break;
        default:
            handled = false;
            break;
        }
        break;
    case VIA_CMD_SET_KEYBOARD_VALUE:
        if (report[1] != VIA_VALUE_LAYOUT_OPTIONS && report[1] != VIA_VALUE_DEVICE_INDICATION) {
            handled = false;
        }
        break;
    case VIA_CMD_CUSTOM_SET_VALUE:
#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW)
        handled = via_rgb_set_value(report);
#else
        handled = false;
#endif
        break;
    case VIA_CMD_CUSTOM_GET_VALUE:
#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW)
        handled = via_rgb_get_value(report);
#else
        handled = false;
#endif
        break;
    case VIA_CMD_CUSTOM_SAVE:
#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW)
        handled = report[1] == VIA_RGBLIGHT_CHANNEL &&
                  zmk_rgb_underglow_save_state() >= 0;
#else
        handled = false;
#endif
        break;
    case VIA_CMD_GET_KEYCODE: {
        uint16_t keycode;
        if (!via_read_keycode(report[1], report[2], report[3], &keycode)) {
            handled = false;
        } else {
            report[4] = keycode >> 8;
            report[5] = keycode;
        }
        break;
    }
    case VIA_CMD_SET_KEYCODE:
        changed = via_write_keycode(report[1], report[2], report[3],
                                    ((uint16_t)report[4] << 8) | report[5]);
        handled = changed;
        break;
    case VIA_CMD_RESET:
        changed = zmk_keymap_reset_settings() >= 0;
        handled = changed;
        break;
    case VIA_CMD_MACRO_GET_COUNT:
        report[1] = 0;
        break;
    case VIA_CMD_MACRO_GET_BUFFER_SIZE:
        report[1] = 0;
        report[2] = 0;
        break;
    case VIA_CMD_MACRO_GET_BUFFER:
        handled = report[3] == 0;
        break;
    case VIA_CMD_MACRO_SET_BUFFER:
        handled = report[3] == 0;
        break;
    case VIA_CMD_MACRO_RESET:
        break;
    case VIA_CMD_GET_LAYER_COUNT:
        report[1] = ZMK_KEYMAP_LAYERS_LEN;
        break;
    case VIA_CMD_GET_BUFFER:
        handled = via_get_buffer(((uint16_t)report[1] << 8) | report[2], report[3], &report[4]);
        break;
    case VIA_CMD_SET_BUFFER:
        handled = via_set_buffer(((uint16_t)report[1] << 8) | report[2], report[3], &report[4]);
        changed = handled && report[3] != 0;
        break;
    case VIA_CMD_GET_ENCODER: {
        uint16_t keycode;
        if (!via_read_encoder(report[1], report[2], report[3] != 0, &keycode)) {
            handled = false;
        } else {
            report[4] = keycode >> 8;
            report[5] = keycode;
        }
        break;
    }
    case VIA_CMD_SET_ENCODER:
        /* Rotation is read-only for now; encoder push switches use keymap commands. */
        handled = report[1] < ZMK_KEYMAP_LAYERS_LEN &&
                  report[2] < ZMK_KEYMAP_SENSORS_LEN && report[3] <= 1;
        break;
    default:
        handled = false;
        break;
    }

    *changed_out = handled && changed;
    if (!handled) {
        report[0] = VIA_CMD_UNHANDLED;
    }
    return handled;
}

static struct k_work via_save_work;
static uint8_t response_report[VIA_REPORT_SIZE];

static void via_save_work_handler(struct k_work *work) {
    (void)work;
    int ret = zmk_keymap_save_changes();
    if (ret < 0) {
        LOG_ERR("Failed to persist VIA keymap changes: %d", ret);
    }
}

static int via_raw_hid_received_listener(const zmk_event_t *event) {
    const struct raw_hid_received_event *received = as_raw_hid_received_event(event);
    if (!received || received->length < 1) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    memset(response_report, 0, sizeof(response_report));
    memcpy(response_report, received->data, MIN(received->length, sizeof(response_report)));

    bool changed = false;
    (void)via_handle_report(response_report, &changed);
    raise_raw_hid_sent_event((struct raw_hid_sent_event){
        .data = response_report,
        .length = VIA_REPORT_SIZE,
    });
    if (changed) {
        (void)k_work_submit(&via_save_work);
    }
    return ZMK_EV_EVENT_HANDLED;
}

ZMK_LISTENER(via_raw_hid_received, via_raw_hid_received_listener);
ZMK_SUBSCRIPTION(via_raw_hid_received, raw_hid_received_event);

static int via_init(void) {
    k_work_init(&via_save_work, via_save_work_handler);
    return 0;
}

SYS_INIT(via_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
