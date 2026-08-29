#include <zephyr/devicetree.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/reboot.h>

#include <dt-bindings/zmk/hid_usage_pages.h>
#include <dt-bindings/zmk/hid_usage.h>
#include <drivers/behavior.h>
#include <dt-bindings/zmk/modifiers.h>
#include <raw_hid/events.h>
#include <via_macro.h>
#include <via_translation.h>
#include <zmk/behavior.h>
#include <zmk/behavior_queue.h>
#include <zmk/keymap.h>
#include <zmk/physical_layouts.h>
#include <zmk/matrix.h>
#include <zmk/sensors.h>
#include <zmk/virtual_key_position.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/rgb_underglow.h>
#include <dt-bindings/zmk/bt.h>
#include <dt-bindings/zmk/ext_power.h>
#include <dt-bindings/zmk/rgb.h>
#include <zmk/hid.h>
#include <dt-bindings/zmk/reset.h>
#include <dt-bindings/zmk/pointing.h>
#include <zmk/events/keycode_state_changed.h>

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
#define VIA_CMD_BOOTLOADER_JUMP 0x0B
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
#define QMK_QK_DEF_LAYER 0x5240
#define QMK_QK_DEF_LAYER_MAX 0x525F
#define QMK_QK_ONE_SHOT_LAYER 0x5280
#define QMK_QK_ONE_SHOT_LAYER_MAX 0x529F
#define QMK_QK_ONE_SHOT_MOD 0x52A0
#define QMK_QK_ONE_SHOT_MOD_MAX 0x52BF
#define QMK_QK_LAYER_TAP_TOGGLE 0x52C0
#define QMK_QK_LAYER_TAP_TOGGLE_MAX 0x52DF
#define QMK_QK_PERSISTENT_DEF_LAYER 0x52E0
#define QMK_QK_PERSISTENT_DEF_LAYER_MAX 0x52FF
#define QMK_QK_TOGGLE_LAYER 0x5260
#define QMK_QK_TOGGLE_LAYER_MAX 0x527F
#define QMK_QK_LAYER_MOD 0x5000
#define QMK_QK_LAYER_MOD_MAX 0x51FF
#define QMK_QK_BASIC_MAX 0x00FF

#define QMK_QK_MACRO 0x7700
#define QMK_QK_MACRO_MAX 0x777F
#define VIA_CUSTOM_KEYCODE_BASE 0x7E00
#define VIA_CUSTOM_KEYCODE_LAST 0x7E0E

#define VIA_ENCODER_LAYER_TAG_MASK 0xF0000000u
#define VIA_ENCODER_LAYER_TAG 0xA0000000u
#define VIA_ENCODER_LAYER_ACTION_SHIFT 24
#define VIA_ENCODER_LAYER_ACTION_MASK 0x0Fu
#define VIA_ENCODER_LAYER_ID_SHIFT 16
#define VIA_ENCODER_LAYER_ID_MASK 0xFFu
#define VIA_ENCODER_LAYER_PAYLOAD_MASK 0xFFFFu

#define VIA_ENCODER_ACTION_LT 1
#define VIA_ENCODER_ACTION_TO 2
#define VIA_ENCODER_ACTION_MO 3
#define VIA_ENCODER_ACTION_TG 4
#define VIA_ENCODER_ACTION_LM 5
#define VIA_ENCODER_ACTION_DF 6
#define VIA_ENCODER_ACTION_PDF 7
#define VIA_ENCODER_ACTION_OSL 8
#define VIA_ENCODER_ACTION_TT 9
#define QMK_QK_MOUSE_MIN 0x00CD
#define QMK_QK_MOUSE_MAX 0x00DF

#define VIA_COMPAT_ACTION_LM 1
#define VIA_COMPAT_ACTION_DF 2
#define VIA_COMPAT_ACTION_PDF 3
#define VIA_COMPAT_ACTION_TT 4
#define VIA_COMPAT_ACTION_OSM 5
#define VIA_COMPAT_ACTION_MOUSE_ACCEL 6
#define VIA_COMPAT_ACTION_MOUSE_MOVE 7
#define VIA_COMPAT_ACTION_SHIFT 24
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
#if DT_NODE_EXISTS(DT_NODELABEL(via_macro))
#define VIA_MACRO_BEHAVIOR DEVICE_DT_NAME(DT_NODELABEL(via_macro))
#else
#define VIA_MACRO_BEHAVIOR ""
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
#if DT_NODE_EXISTS(DT_NODELABEL(via_encoder))
#define VIA_ENCODER_BEHAVIOR DEVICE_DT_NAME(DT_NODELABEL(via_encoder))
#else
#define VIA_ENCODER_BEHAVIOR ""
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(via_compat))
#define VIA_COMPAT_BEHAVIOR DEVICE_DT_NAME(DT_NODELABEL(via_compat))
#else
#define VIA_COMPAT_BEHAVIOR ""
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(sl))
#define VIA_SL_BEHAVIOR DEVICE_DT_NAME(DT_NODELABEL(sl))
#else
#define VIA_SL_BEHAVIOR ""
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(mmv))
#define VIA_MMV_BEHAVIOR DEVICE_DT_NAME(DT_NODELABEL(mmv))
#else
#define VIA_MMV_BEHAVIOR ""
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(msc))
#define VIA_MSC_BEHAVIOR DEVICE_DT_NAME(DT_NODELABEL(msc))
#else
#define VIA_MSC_BEHAVIOR ""
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(mkp))
#define VIA_MKP_BEHAVIOR DEVICE_DT_NAME(DT_NODELABEL(mkp))
#else
#define VIA_MKP_BEHAVIOR ""
#endif

BUILD_ASSERT(VIA_REPORT_SIZE == 32, "Stock VIA requires 32-byte Raw HID reports");
BUILD_ASSERT((QMK_QK_MOD_TAP | ((uint16_t)BIT(1) << 8) | 0x04) == 0x2204,
             "QMK LSHIFT_T(KC_A) encoding changed");
BUILD_ASSERT((QMK_QK_MOD_TAP | ((uint16_t)BIT(0) << 8) | 0x06) == 0x2106,
             "QMK LCTL_T(KC_C) encoding changed");
BUILD_ASSERT((QMK_QK_MOD_TAP | ((uint16_t)(BIT(2) | BIT(4)) << 8) | 0x1B) == 0x341B,
             "QMK RALT_T(KC_X) encoding changed");

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
static bool via_zmk_mods_to_qmk(uint8_t zmk_mods, uint8_t *qmk_mods);
static bool via_qmk_mod_to_mt_usage(uint8_t qmk_mods, uint32_t *usage) {
    const uint8_t types = qmk_mods & 0x0F;
    const bool right = (qmk_mods & BIT(4)) != 0;
    const uint8_t base_type = types & (uint8_t)(-types);
    const uint8_t extra_types = types ^ base_type;
    uint8_t usage_id;

    if (types == 0) {
        return false;
    }

    switch (base_type) {
    case BIT(0):
        usage_id = 0xE0;
        break;
    case BIT(1):
        usage_id = 0xE1;
        break;
    case BIT(2):
        usage_id = 0xE2;
        break;
    case BIT(3):
        usage_id = 0xE3;
        break;
    default:
        return false;
    }

    const uint8_t extra_qmk_mods = extra_types | (extra_types && right ? BIT(4) : 0);
    *usage = ((uint32_t)via_qmk_mods_to_zmk(extra_qmk_mods) << 24) |
             ZMK_HID_USAGE(HID_USAGE_KEY, usage_id + (right ? 4 : 0));
    return true;
}

static bool via_mt_usage_to_qmk_mods(uint32_t usage, uint8_t *qmk_mods) {
    uint8_t extra_qmk_mods;
    uint8_t base_qmk_mod;
    const uint16_t usage_id = ZMK_HID_USAGE_ID(usage);

    if (ZMK_HID_USAGE_PAGE(usage) != HID_USAGE_KEY ||
        !via_zmk_mods_to_qmk(usage >> 24, &extra_qmk_mods)) {
        return false;
    }

    switch (usage_id) {
    case 0xE0:
        base_qmk_mod = BIT(0);
        break;
    case 0xE1:
        base_qmk_mod = BIT(1);
        break;
    case 0xE2:
        base_qmk_mod = BIT(2);
        break;
    case 0xE3:
        base_qmk_mod = BIT(3);
        break;
    case 0xE4:
        base_qmk_mod = BIT(0) | BIT(4);
        break;
    case 0xE5:
        base_qmk_mod = BIT(1) | BIT(4);
        break;
    case 0xE6:
        base_qmk_mod = BIT(2) | BIT(4);
        break;
    case 0xE7:
        base_qmk_mod = BIT(3) | BIT(4);
        break;
    default:
        return false;
    }

    if (extra_qmk_mods != 0 &&
        ((extra_qmk_mods & BIT(4)) != (base_qmk_mod & BIT(4)) ||
         (extra_qmk_mods & base_qmk_mod & 0x0F) != 0)) {
        return false;
    }

    *qmk_mods = base_qmk_mod | extra_qmk_mods;
    return true;
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

static zmk_mod_flags_t via_oneshot_mods;
static uint8_t via_mouse_acceleration;
static uint8_t via_mouse_move_acceleration[ZMK_KEYMAP_LEN];
static uint32_t via_tt_press_time[ZMK_KEYMAP_LEN];
static uint32_t via_tt_last_tap_time[ZMK_KEYMAP_LEN];
static uint8_t via_tt_tap_count[ZMK_KEYMAP_LEN];
static bool via_tt_sequence_active[ZMK_KEYMAP_LEN];
static bool via_tt_was_active[ZMK_KEYMAP_LEN];
static void via_oneshot_release_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(via_oneshot_release_work, via_oneshot_release_work_handler);

static void via_oneshot_release_work_handler(struct k_work *work) {
    ARG_UNUSED(work);
    if (via_oneshot_mods) {
        zmk_hid_unregister_mods(via_oneshot_mods);
        via_oneshot_mods = 0;
    }
}
static int via_compat_keycode_listener(const zmk_event_t *event) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(event);
    if (ev && ev->state && via_oneshot_mods &&
        !(ev->usage_page == HID_USAGE_KEY && ev->keycode >= 0xE0 && ev->keycode <= 0xE7)) {
        k_work_reschedule(&via_oneshot_release_work, K_NO_WAIT);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

static void via_mouse_move_vector(uint8_t value, int16_t *dx, int16_t *dy) {
    *dx = 0;
    *dy = 0;
    switch (value) {
    case 0: *dy = MOVE_Y_DECODE(MOVE_UP); break;
    case 1: *dy = MOVE_Y_DECODE(MOVE_DOWN); break;
    case 2: *dx = MOVE_X_DECODE(MOVE_LEFT); break;
    case 3: *dx = MOVE_X_DECODE(MOVE_RIGHT); break;
    default: break;
    }
}

static int via_compat_binding_pressed(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    const uint8_t action = binding->param1 >> VIA_COMPAT_ACTION_SHIFT;
    const uint8_t value = binding->param1 & 0xFF;
    switch (action) {
    case VIA_COMPAT_ACTION_LM:
        if (zmk_keymap_layer_activate(value) < 0) {
            return -EINVAL;
        }
        return zmk_hid_register_mods(via_qmk_mods_to_zmk(binding->param2 >> 24));
    case VIA_COMPAT_ACTION_DF:
        return zmk_keymap_set_default_layer(value, false);
    case VIA_COMPAT_ACTION_PDF:
        return zmk_keymap_set_default_layer(value, true);
    case VIA_COMPAT_ACTION_OSM:
        via_oneshot_mods = via_qmk_mods_to_zmk(value);
        k_work_reschedule(&via_oneshot_release_work, K_MSEC(5000));
        return zmk_hid_register_mods(via_oneshot_mods);
    case VIA_COMPAT_ACTION_MOUSE_ACCEL:
        via_mouse_acceleration = MIN(value, 2);
        return 0;
    case VIA_COMPAT_ACTION_MOUSE_MOVE: {
        if (event.position >= ZMK_KEYMAP_LEN) return -EINVAL;
        via_mouse_move_acceleration[event.position] = via_mouse_acceleration;
        const int16_t speed = 1 << via_mouse_move_acceleration[event.position];
        int16_t dx;
        int16_t dy;
        via_mouse_move_vector(value, &dx, &dy);
        dx *= speed;
        dy *= speed;
        extern int behavior_input_two_axis_adjust_speed(const struct device *, int16_t, int16_t);
        return behavior_input_two_axis_adjust_speed(zmk_behavior_get_binding(VIA_MMV_BEHAVIOR),
                                                    dx, dy);
    }
    case VIA_COMPAT_ACTION_TT:
        if (event.position >= ZMK_KEYMAP_LEN) {
            return -EINVAL;
        }
        if (!via_tt_sequence_active[event.position] ||
            event.timestamp - via_tt_last_tap_time[event.position] > 500) {
            via_tt_tap_count[event.position] = 0;
            via_tt_was_active[event.position] = zmk_keymap_layer_active(value);
        }
        via_tt_sequence_active[event.position] = true;
        via_tt_press_time[event.position] = event.timestamp;
        return zmk_keymap_layer_activate(value);
    default:
        return -ENOTSUP;
    }
}

static int via_compat_binding_released(struct zmk_behavior_binding *binding,
                                       struct zmk_behavior_binding_event event) {
    const uint8_t action = binding->param1 >> VIA_COMPAT_ACTION_SHIFT;
    const uint8_t value = binding->param1 & 0xFF;
    switch (action) {
    case VIA_COMPAT_ACTION_LM:
        zmk_hid_unregister_mods(via_qmk_mods_to_zmk(binding->param2 >> 24));
        return zmk_keymap_layer_deactivate(value);
    case VIA_COMPAT_ACTION_MOUSE_MOVE: {
        if (event.position >= ZMK_KEYMAP_LEN) return -EINVAL;
        const int16_t speed = 1 << via_mouse_move_acceleration[event.position];
        int16_t dx;
        int16_t dy;
        via_mouse_move_vector(value, &dx, &dy);
        dx *= -speed;
        dy *= -speed;
        extern int behavior_input_two_axis_adjust_speed(const struct device *, int16_t, int16_t);
        return behavior_input_two_axis_adjust_speed(zmk_behavior_get_binding(VIA_MMV_BEHAVIOR),
                                                    dx, dy);
    }
    case VIA_COMPAT_ACTION_TT:
        if (event.position >= ZMK_KEYMAP_LEN) {
            return -EINVAL;
        }
        if (event.timestamp - via_tt_press_time[event.position] > 200) {
            via_tt_sequence_active[event.position] = false;
            via_tt_tap_count[event.position] = 0;
            return zmk_keymap_layer_deactivate(value);
        }
        via_tt_last_tap_time[event.position] = event.timestamp;
        if (++via_tt_tap_count[event.position] < 5) {
            return zmk_keymap_layer_deactivate(value);
        }
        via_tt_sequence_active[event.position] = false;
        via_tt_tap_count[event.position] = 0;
        const bool target_active = !via_tt_was_active[event.position];
        if (zmk_keymap_layer_active(value) != target_active) {
            return zmk_keymap_layer_toggle(value);
        }
        return 0;
    default:
        return 0;
    }
}

static const struct behavior_driver_api via_compat_driver_api = {
    .binding_pressed = via_compat_binding_pressed,
    .binding_released = via_compat_binding_released,
};

#if DT_NODE_EXISTS(DT_NODELABEL(via_compat))
BEHAVIOR_DT_DEFINE(DT_NODELABEL(via_compat), NULL, NULL, NULL, NULL, POST_KERNEL,
                   CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &via_compat_driver_api);
ZMK_LISTENER(via_compat, via_compat_keycode_listener);
ZMK_SUBSCRIPTION(via_compat, zmk_keycode_state_changed);
#endif
static struct zmk_keymap_layer_order_snapshot via_layer_order_snapshot;
static bool via_layer_order_snapshot_valid;

static bool via_layer_index_to_id(uint8_t index, zmk_keymap_layer_id_t *id) {
    if (index >= ZMK_KEYMAP_LAYERS_LEN) {
        return false;
    }
    if (via_layer_order_snapshot_valid) {
        *id = via_layer_order_snapshot.index_to_id[index];
    } else {
        *id = zmk_keymap_layer_index_to_id(index);
    }
    return *id != ZMK_KEYMAP_LAYER_ID_INVAL;
}

static bool via_layer_id_to_index(zmk_keymap_layer_id_t id, uint8_t *index) {
    if (id >= ZMK_KEYMAP_LAYERS_LEN) {
        return false;
    }
    if (via_layer_order_snapshot_valid) {
        *index = via_layer_order_snapshot.id_to_index[id];
        return *index != ZMK_KEYMAP_LAYER_ID_INVAL;
    }
    for (uint8_t candidate = 0; candidate < ZMK_KEYMAP_LAYERS_LEN; candidate++) {
        if (zmk_keymap_layer_index_to_id(candidate) == id) {
            *index = candidate;
            return true;
        }
    }
    return false;
}

bool via_qmk_basic_to_usage(uint8_t keycode, uint32_t *usage) {
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
    case 0xAF: /* KC_MEDIA_SELECT */
        *usage = ZMK_HID_USAGE(HID_USAGE_CONSUMER, 0x183);
        return true;
    case 0xB0: /* KC_MEDIA_EJECT */
        *usage = ZMK_HID_USAGE(HID_USAGE_CONSUMER, 0xB8);
        return true;
    case 0xB1: /* KC_MAIL */
        *usage = ZMK_HID_USAGE(HID_USAGE_CONSUMER, 0x18A);
        return true;
    case 0xB2: /* KC_CALCULATOR */
        *usage = ZMK_HID_USAGE(HID_USAGE_CONSUMER, 0x192);
        return true;
    case 0xB3: /* KC_MY_COMPUTER */
        *usage = ZMK_HID_USAGE(HID_USAGE_CONSUMER, 0x194);
        return true;
    case 0xB4: /* KC_WWW_SEARCH */
        *usage = ZMK_HID_USAGE(HID_USAGE_CONSUMER, 0x221);
        return true;
    case 0xB5: /* KC_WWW_HOME */
        *usage = ZMK_HID_USAGE(HID_USAGE_CONSUMER, 0x223);
        return true;
    case 0xB6: /* KC_WWW_BACK */
        *usage = ZMK_HID_USAGE(HID_USAGE_CONSUMER, 0x224);
        return true;
    case 0xB7: /* KC_WWW_FORWARD */
        *usage = ZMK_HID_USAGE(HID_USAGE_CONSUMER, 0x225);
        return true;
    case 0xB8: /* KC_WWW_STOP */
        *usage = ZMK_HID_USAGE(HID_USAGE_CONSUMER, 0x226);
        return true;
    case 0xB9: /* KC_WWW_REFRESH */
        *usage = ZMK_HID_USAGE(HID_USAGE_CONSUMER, 0x227);
        return true;
    case 0xBA: /* KC_WWW_FAVORITES */
        *usage = ZMK_HID_USAGE(HID_USAGE_CONSUMER, 0x22A);
        return true;
    case 0xBB: /* KC_MEDIA_FAST_FORWARD */
        *usage = ZMK_HID_USAGE(HID_USAGE_CONSUMER, 0xB3);
        return true;
    case 0xBC: /* KC_MEDIA_REWIND */
        *usage = ZMK_HID_USAGE(HID_USAGE_CONSUMER, 0xB4);
        return true;
    case 0xBD: /* KC_BRIGHTNESS_UP */
        *usage = ZMK_HID_USAGE(HID_USAGE_CONSUMER, 0x6F);
        return true;
    case 0xBE: /* KC_BRIGHTNESS_DOWN */
        *usage = ZMK_HID_USAGE(HID_USAGE_CONSUMER, 0x70);
        return true;
    case 0xBF: /* KC_CONTROL_PANEL */
        *usage = ZMK_HID_USAGE(HID_USAGE_CONSUMER, 0x19F);
        return true;
    case 0xC0: /* KC_ASSISTANT */
        *usage = ZMK_HID_USAGE(HID_USAGE_CONSUMER, 0x1CB);
        return true;
    case 0xC1: /* KC_MISSION_CONTROL */
        *usage = ZMK_HID_USAGE(HID_USAGE_CONSUMER, 0x29F);
        return true;
    case 0xC2: /* KC_LAUNCHPAD */
        *usage = ZMK_HID_USAGE(HID_USAGE_CONSUMER, 0x2A0);
        return true;
    default:
        break;
    }

    /* QMK mouse keycodes 0xCD..0xDF need ZMK mouse behaviors, not &kp. */
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
        case 0x183:
            *keycode = 0xAF;
            return true;
        case 0xB8:
            *keycode = 0xB0;
            return true;
        case 0x18A:
            *keycode = 0xB1;
            return true;
        case 0x192:
            *keycode = 0xB2;
            return true;
        case 0x194:
            *keycode = 0xB3;
            return true;
        case 0x221:
            *keycode = 0xB4;
            return true;
        case 0x223:
            *keycode = 0xB5;
            return true;
        case 0x224:
            *keycode = 0xB6;
            return true;
        case 0x225:
            *keycode = 0xB7;
            return true;
        case 0x226:
            *keycode = 0xB8;
            return true;
        case 0x227:
            *keycode = 0xB9;
            return true;
        case 0x22A:
            *keycode = 0xBA;
            return true;
        case 0xB3:
            *keycode = 0xBB;
            return true;
        case 0xB4:
            *keycode = 0xBC;
            return true;
        case 0x6F:
            *keycode = 0xBD;
            return true;
        case 0x70:
            *keycode = 0xBE;
            return true;
        case 0x19F:
            *keycode = 0xBF;
            return true;
        case 0x1CB:
            *keycode = 0xC0;
            return true;
        case 0x29F:
            *keycode = 0xC1;
            return true;
        case 0x2A0:
            *keycode = 0xC2;
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

static bool via_qmk_mouse_to_binding(uint8_t keycode, struct zmk_behavior_binding *binding) {
    uint8_t action = 0;
    uint8_t value = 0;
    switch (keycode) {
    case 0xCD: value = 0; break;
    case 0xCE: value = 1; break;
    case 0xCF: value = 2; break;
    case 0xD0: value = 3; break;
    case 0xD1: case 0xD2: case 0xD3: case 0xD4:
    case 0xD5: case 0xD6: case 0xD7: case 0xD8:
        if (!VIA_MKP_BEHAVIOR[0]) {
            return false;
        }
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = VIA_MKP_BEHAVIOR,
            .param1 = BIT(keycode - 0xD1),
        };
        return true;
    case 0xD9:
        *binding = (struct zmk_behavior_binding){.behavior_dev = VIA_MSC_BEHAVIOR,
                                                  .param1 = SCRL_UP};
        return true;
    case 0xDA:
        *binding = (struct zmk_behavior_binding){.behavior_dev = VIA_MSC_BEHAVIOR,
                                                  .param1 = SCRL_DOWN};
        return true;
    case 0xDB:
        *binding = (struct zmk_behavior_binding){.behavior_dev = VIA_MSC_BEHAVIOR,
                                                  .param1 = SCRL_LEFT};
        return true;
    case 0xDC:
        *binding = (struct zmk_behavior_binding){.behavior_dev = VIA_MSC_BEHAVIOR,
                                                  .param1 = SCRL_RIGHT};
        return true;
    case 0xDD: case 0xDE: case 0xDF:
        action = VIA_COMPAT_ACTION_MOUSE_ACCEL;
        value = keycode - 0xDD;
        break;
    default:
        return false;
    }
    if (!VIA_COMPAT_BEHAVIOR[0]) {
        return false;
    }
    if (!action) {
        action = VIA_COMPAT_ACTION_MOUSE_MOVE;
    }
    *binding = (struct zmk_behavior_binding){
        .behavior_dev = VIA_COMPAT_BEHAVIOR,
        .param1 = (action << VIA_COMPAT_ACTION_SHIFT) | value,
    };
    return true;
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
    if (keycode >= QMK_QK_MACRO && keycode <= QMK_QK_MACRO_MAX) {
        const uint16_t id = keycode - QMK_QK_MACRO;
        if (id >= zmk_via_macro_get_count() || !VIA_MACRO_BEHAVIOR[0]) {
            return false;
        }
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = VIA_MACRO_BEHAVIOR,
            .param1 = id,
        };
        return true;
    }
    if (via_custom_keycode_to_binding(keycode, binding)) {
        return true;
    }
    if (via_translation_classify_keycode(keycode) == VIA_TRANSLATION_UNSUPPORTED) {
        return false;
    }
    if (keycode <= QMK_QK_BASIC_MAX && via_qmk_mouse_to_binding((uint8_t)keycode, binding)) {
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

    if (keycode >= QMK_QK_LAYER_MOD && keycode <= QMK_QK_LAYER_MOD_MAX) {
        zmk_keymap_layer_id_t layer_id;
        if (!via_layer_index_to_id((keycode >> 5) & 0x0F, &layer_id) || !VIA_COMPAT_BEHAVIOR[0]) {
            return false;
        }
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = VIA_COMPAT_BEHAVIOR,
            .param1 = (VIA_COMPAT_ACTION_LM << VIA_COMPAT_ACTION_SHIFT) | layer_id,
            .param2 = (keycode & 0x1F) << 24,
        };
        return true;
    }
    if (keycode >= QMK_QK_DEF_LAYER && keycode <= QMK_QK_DEF_LAYER_MAX) {
        zmk_keymap_layer_id_t layer_id;
        if (!via_layer_index_to_id(keycode & 0x1F, &layer_id) || !VIA_COMPAT_BEHAVIOR[0]) {
            return false;
        }
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = VIA_COMPAT_BEHAVIOR,
            .param1 = (VIA_COMPAT_ACTION_DF << VIA_COMPAT_ACTION_SHIFT) | layer_id,
        };
        return true;
    }
    if (keycode >= QMK_QK_PERSISTENT_DEF_LAYER && keycode <= QMK_QK_PERSISTENT_DEF_LAYER_MAX) {
        zmk_keymap_layer_id_t layer_id;
        if (!via_layer_index_to_id(keycode & 0x1F, &layer_id) || !VIA_COMPAT_BEHAVIOR[0]) {
            return false;
        }
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = VIA_COMPAT_BEHAVIOR,
            .param1 = (VIA_COMPAT_ACTION_PDF << VIA_COMPAT_ACTION_SHIFT) | layer_id,
        };
        return true;
    }
    if (keycode >= QMK_QK_ONE_SHOT_LAYER && keycode <= QMK_QK_ONE_SHOT_LAYER_MAX) {
        zmk_keymap_layer_id_t layer_id;
        if (!via_layer_index_to_id(keycode & 0x1F, &layer_id) || !VIA_SL_BEHAVIOR[0]) {
            return false;
        }
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = VIA_SL_BEHAVIOR,
            .param1 = layer_id,
        };
        return true;
    }
    if (keycode >= QMK_QK_ONE_SHOT_MOD && keycode <= QMK_QK_ONE_SHOT_MOD_MAX) {
        if (!VIA_COMPAT_BEHAVIOR[0]) {
            return false;
        }
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = VIA_COMPAT_BEHAVIOR,
            .param1 = (VIA_COMPAT_ACTION_OSM << VIA_COMPAT_ACTION_SHIFT) |
                      (keycode & 0x1F),
        };
        return true;
    }
    if (keycode >= QMK_QK_LAYER_TAP_TOGGLE && keycode <= QMK_QK_LAYER_TAP_TOGGLE_MAX) {
        zmk_keymap_layer_id_t layer_id;
        if (!via_layer_index_to_id(keycode & 0x1F, &layer_id) || !VIA_COMPAT_BEHAVIOR[0]) {
            return false;
        }
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = VIA_COMPAT_BEHAVIOR,
            .param1 = (VIA_COMPAT_ACTION_TT << VIA_COMPAT_ACTION_SHIFT) | layer_id,
        };
        return true;
    }

    if (keycode >= QMK_QK_MOD_TAP && keycode <= QMK_QK_MOD_TAP_MAX) {
        if (!via_qmk_basic_to_usage(keycode & 0xFF, &usage) ||
            !via_qmk_mod_to_mt_usage((keycode >> 8) & 0x1F, &binding->param1)) {
            return false;
        }
        binding->behavior_dev = VIA_MT_BEHAVIOR;
        binding->param2 = usage;
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
    if (keycode > QMK_QK_BASIC_MAX ||
        !via_qmk_basic_to_usage((uint8_t)keycode, &usage)) {
        return false;
    }
    *binding = (struct zmk_behavior_binding){
        .behavior_dev = VIA_KP_BEHAVIOR,
        .param1 = usage,
    };
    return true;
}

static bool via_compat_binding_to_qmk(const struct zmk_behavior_binding *binding,
                                      uint16_t *keycode) {
    if (!binding || !binding->behavior_dev ||
        strcmp(binding->behavior_dev, VIA_COMPAT_BEHAVIOR) != 0) {
        return false;
    }

    const uint8_t action = binding->param1 >> VIA_COMPAT_ACTION_SHIFT;
    const zmk_keymap_layer_id_t layer_id = binding->param1 & 0xFF;
    uint8_t layer_index;
    if ((action == VIA_COMPAT_ACTION_LM || action == VIA_COMPAT_ACTION_DF ||
         action == VIA_COMPAT_ACTION_PDF || action == VIA_COMPAT_ACTION_TT) &&
        !via_layer_id_to_index(layer_id, &layer_index)) {
        return false;
    }

    switch (action) {
    case VIA_COMPAT_ACTION_LM:
        if (layer_index >= 16 || (binding->param2 >> 24) > 0x1F) {
            return false;
        }
        *keycode = QMK_QK_LAYER_MOD | ((uint16_t)layer_index << 5) |
                   ((binding->param2 >> 24) & 0x1F);
        return true;
    case VIA_COMPAT_ACTION_DF:
        *keycode = QMK_QK_DEF_LAYER | layer_index;
        return true;
    case VIA_COMPAT_ACTION_PDF:
        *keycode = QMK_QK_PERSISTENT_DEF_LAYER | layer_index;
        return true;
    case VIA_COMPAT_ACTION_TT:
        *keycode = QMK_QK_LAYER_TAP_TOGGLE | layer_index;
        return true;
    case VIA_COMPAT_ACTION_OSM:
        *keycode = QMK_QK_ONE_SHOT_MOD | (binding->param1 & 0x1F);
        return true;
    case VIA_COMPAT_ACTION_MOUSE_MOVE:
        if ((binding->param1 & 0xFF) > 3) {
            return false;
        }
        *keycode = QMK_QK_MOUSE_MIN + (binding->param1 & 0xFF);
        return true;
    case VIA_COMPAT_ACTION_MOUSE_ACCEL:
        if ((binding->param1 & 0xFF) > 2) {
            return false;
        }
        *keycode = QMK_QK_MOUSE_MIN + 0x10 + (binding->param1 & 0xFF);
        return true;
    default:
        return false;
    }
}

static bool via_mouse_binding_to_qmk(const struct zmk_behavior_binding *binding,
                                     uint16_t *keycode) {
    if (!binding || !binding->behavior_dev) {
        return false;
    }
    if (strcmp(binding->behavior_dev, VIA_MMV_BEHAVIOR) == 0) {
        switch (binding->param1) {
        case MOVE_UP: *keycode = 0xCD; return true;
        case MOVE_DOWN: *keycode = 0xCE; return true;
        case MOVE_LEFT: *keycode = 0xCF; return true;
        case MOVE_RIGHT: *keycode = 0xD0; return true;
        default: return false;
        }
    }
    if (strcmp(binding->behavior_dev, VIA_MSC_BEHAVIOR) == 0) {
        switch (binding->param1) {
        case SCRL_UP: *keycode = 0xD9; return true;
        case SCRL_DOWN: *keycode = 0xDA; return true;
        case SCRL_LEFT: *keycode = 0xDB; return true;
        case SCRL_RIGHT: *keycode = 0xDC; return true;
        default: return false;
        }
    }
    if (strcmp(binding->behavior_dev, VIA_MKP_BEHAVIOR) == 0 &&
        binding->param1 && (binding->param1 & (binding->param1 - 1)) == 0 &&
        binding->param1 <= BIT(7) && binding->param2 == 0) {
        *keycode = 0xD1 + __builtin_ctz(binding->param1);
        return true;
    }
    return false;
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
    if (strcmp(binding->behavior_dev, VIA_MACRO_BEHAVIOR) == 0 &&
        binding->param1 < zmk_via_macro_get_count()) {
        *keycode = QMK_QK_MACRO + binding->param1;
        return true;
    }
    if (via_binding_to_custom_keycode(binding, keycode) ||
        via_compat_binding_to_qmk(binding, keycode) ||
        via_mouse_binding_to_qmk(binding, keycode)) {
        return true;
    }
    if (strcmp(binding->behavior_dev, VIA_SL_BEHAVIOR) == 0 &&
        via_layer_id_to_index(binding->param1, &basic) && basic < 32) {
        *keycode = QMK_QK_ONE_SHOT_LAYER | basic;
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
    if (strcmp(binding->behavior_dev, VIA_MT_BEHAVIOR) == 0) {
        if ((binding->param2 >> 24) == 0 &&
            via_usage_to_qmk_basic(binding->param2, &basic) &&
            via_mt_usage_to_qmk_mods(binding->param1, &qmk_mods)) {
            *keycode = QMK_QK_MOD_TAP | ((uint16_t)qmk_mods << 8) | basic;
            return true;
        }
    }
    if (strcmp(binding->behavior_dev, VIA_LT_BEHAVIOR) == 0) {
        uint8_t layer_index;
        if (via_layer_id_to_index(binding->param1, &layer_index) &&
            layer_index < 16 && (binding->param2 >> 24) == 0 &&
            via_usage_to_qmk_basic(binding->param2, &basic)) {
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

static bool via_make_binding(uint16_t keycode, struct zmk_behavior_binding *binding);
static bool via_encoder_layer_param_to_qmk(uint32_t param, uint16_t *keycode) {
    const uint8_t action =
        (param >> VIA_ENCODER_LAYER_ACTION_SHIFT) & VIA_ENCODER_LAYER_ACTION_MASK;
    const zmk_keymap_layer_id_t layer_id =
        (param >> VIA_ENCODER_LAYER_ID_SHIFT) & VIA_ENCODER_LAYER_ID_MASK;
    uint8_t layer_index;

    if ((param & VIA_ENCODER_LAYER_TAG_MASK) != VIA_ENCODER_LAYER_TAG ||
        !via_layer_id_to_index(layer_id, &layer_index)) {
        return false;
    }

    switch (action) {
    case VIA_ENCODER_ACTION_LT:
        if ((param & VIA_ENCODER_LAYER_PAYLOAD_MASK) > 0xFF || layer_index >= 16) {
            return false;
        }
        *keycode = QMK_QK_LAYER_TAP | ((uint16_t)layer_index << 8) |
                   (param & VIA_ENCODER_LAYER_PAYLOAD_MASK);
        return true;
    case VIA_ENCODER_ACTION_TO:
        if (param & VIA_ENCODER_LAYER_PAYLOAD_MASK || layer_index >= 32) {
            return false;
        }
        *keycode = QMK_QK_TO | layer_index;
        return true;
    case VIA_ENCODER_ACTION_MO:
        if (param & VIA_ENCODER_LAYER_PAYLOAD_MASK || layer_index >= 32) {
            return false;
        }
        *keycode = QMK_QK_MOMENTARY | layer_index;
        return true;
    case VIA_ENCODER_ACTION_TG:
        if (param & VIA_ENCODER_LAYER_PAYLOAD_MASK || layer_index >= 32) {
            return false;
        }
        *keycode = QMK_QK_TOGGLE_LAYER | layer_index;
        return true;
    case VIA_ENCODER_ACTION_LM:
        if (layer_index >= 16 || (param & VIA_ENCODER_LAYER_PAYLOAD_MASK) > 0x1F) {
            return false;
        }
        *keycode = QMK_QK_LAYER_MOD | ((uint16_t)layer_index << 5) |
                   (param & VIA_ENCODER_LAYER_PAYLOAD_MASK);
        return true;
    case VIA_ENCODER_ACTION_DF:
        *keycode = QMK_QK_DEF_LAYER | layer_index;
        return true;
    case VIA_ENCODER_ACTION_PDF:
        *keycode = QMK_QK_PERSISTENT_DEF_LAYER | layer_index;
        return true;
    case VIA_ENCODER_ACTION_OSL:
        *keycode = QMK_QK_ONE_SHOT_LAYER | layer_index;
        return true;
    case VIA_ENCODER_ACTION_TT:
        *keycode = QMK_QK_LAYER_TAP_TOGGLE | layer_index;
        return true;
    default:
        return false;
    }
}

static bool via_encoder_param_to_qmk(uint32_t param, uint16_t *keycode) {
    if ((param & VIA_ENCODER_LAYER_TAG_MASK) == VIA_ENCODER_LAYER_TAG) {
        return via_encoder_layer_param_to_qmk(param, keycode);
    }
    if (param > UINT16_MAX) {
        return false;
    }
    *keycode = param;
    return true;
}

static bool via_encoder_param_to_binding(uint32_t param,
                                         struct zmk_behavior_binding *binding) {
    uint16_t keycode;

    return via_encoder_param_to_qmk(param, &keycode) && via_make_binding(keycode, binding);
}

static bool via_normalize_encoder_keycode(uint16_t keycode, uint32_t *param) {
    zmk_keymap_layer_id_t layer_id;
    uint8_t action;
    uint8_t layer_index;

    *param = keycode;
    if (keycode >= QMK_QK_LAYER_TAP && keycode <= QMK_QK_LAYER_TAP_MAX) {
        action = VIA_ENCODER_ACTION_LT;
        layer_index = (keycode >> 8) & 0x0F;
    } else if (keycode >= QMK_QK_TO && keycode <= QMK_QK_TO_MAX) {
        action = VIA_ENCODER_ACTION_TO;
        layer_index = keycode & 0x1F;
    } else if (keycode >= QMK_QK_MOMENTARY && keycode <= QMK_QK_MOMENTARY_MAX) {
        action = VIA_ENCODER_ACTION_MO;
        layer_index = keycode & 0x1F;
    } else if (keycode >= QMK_QK_TOGGLE_LAYER && keycode <= QMK_QK_TOGGLE_LAYER_MAX) {
        action = VIA_ENCODER_ACTION_TG;
        layer_index = keycode & 0x1F;
    } else if (keycode >= QMK_QK_LAYER_MOD && keycode <= QMK_QK_LAYER_MOD_MAX) {
        action = VIA_ENCODER_ACTION_LM;
        layer_index = (keycode >> 5) & 0x0F;
    } else if (keycode >= QMK_QK_DEF_LAYER && keycode <= QMK_QK_DEF_LAYER_MAX) {
        action = VIA_ENCODER_ACTION_DF;
        layer_index = keycode & 0x1F;
    } else if (keycode >= QMK_QK_PERSISTENT_DEF_LAYER &&
               keycode <= QMK_QK_PERSISTENT_DEF_LAYER_MAX) {
        action = VIA_ENCODER_ACTION_PDF;
        layer_index = keycode & 0x1F;
    } else if (keycode >= QMK_QK_ONE_SHOT_LAYER && keycode <= QMK_QK_ONE_SHOT_LAYER_MAX) {
        action = VIA_ENCODER_ACTION_OSL;
        layer_index = keycode & 0x1F;
    } else if (keycode >= QMK_QK_LAYER_TAP_TOGGLE &&
               keycode <= QMK_QK_LAYER_TAP_TOGGLE_MAX) {
        action = VIA_ENCODER_ACTION_TT;
        layer_index = keycode & 0x1F;
    } else {
        return true;
    }

    if (!via_layer_index_to_id(layer_index, &layer_id)) {
        return false;
    }

    const uint16_t payload = (action == VIA_ENCODER_ACTION_LT) ? (keycode & 0xFFu) :
                             (action == VIA_ENCODER_ACTION_LM) ? (keycode & 0x1Fu) : 0u;
    *param = via_encoder_tag_encode(action, layer_id, payload);
    return true;
}

static bool via_encoder_tagged_param_to_binding(uint32_t param,
                                                struct zmk_behavior_binding *binding) {
    struct via_encoder_tag tag;
    if (!via_encoder_tag_decode(param, &tag) || tag.layer_id >= ZMK_KEYMAP_LAYERS_LEN) {
        return false;
    }
    const uint8_t action = tag.action;
    const zmk_keymap_layer_id_t layer_id = tag.layer_id;

    switch (action) {
    case VIA_ENCODER_ACTION_LT:
        if ((param & VIA_ENCODER_LAYER_PAYLOAD_MASK) > QMK_QK_BASIC_MAX) {
            return false;
        }
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = VIA_LT_BEHAVIOR,
            .param1 = layer_id,
            .param2 = param & VIA_ENCODER_LAYER_PAYLOAD_MASK,
        };
        return true;
    case VIA_ENCODER_ACTION_TO:
        if (param & VIA_ENCODER_LAYER_PAYLOAD_MASK) {
            return false;
        }
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = VIA_TO_BEHAVIOR,
            .param1 = layer_id,
        };
        return true;
    case VIA_ENCODER_ACTION_MO:
        if (param & VIA_ENCODER_LAYER_PAYLOAD_MASK) {
            return false;
        }
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = VIA_MO_BEHAVIOR,
            .param1 = layer_id,
        };
        return true;
    case VIA_ENCODER_ACTION_TG:
        if (param & VIA_ENCODER_LAYER_PAYLOAD_MASK) {
            return false;
        }
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = VIA_TOG_BEHAVIOR,
            .param1 = layer_id,
        };
        return true;
    case VIA_ENCODER_ACTION_LM:
        if ((param & VIA_ENCODER_LAYER_PAYLOAD_MASK) > 0x1F) {
            return false;
        }
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = VIA_COMPAT_BEHAVIOR,
            .param1 = (VIA_COMPAT_ACTION_LM << VIA_COMPAT_ACTION_SHIFT) | layer_id,
            .param2 = (param & VIA_ENCODER_LAYER_PAYLOAD_MASK) << 24,
        };
        return true;
    case VIA_ENCODER_ACTION_DF:
        if (param & VIA_ENCODER_LAYER_PAYLOAD_MASK) return false;
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = VIA_COMPAT_BEHAVIOR,
            .param1 = (VIA_COMPAT_ACTION_DF << VIA_COMPAT_ACTION_SHIFT) | layer_id};
        return true;
    case VIA_ENCODER_ACTION_PDF:
        if (param & VIA_ENCODER_LAYER_PAYLOAD_MASK) return false;
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = VIA_COMPAT_BEHAVIOR,
            .param1 = (VIA_COMPAT_ACTION_PDF << VIA_COMPAT_ACTION_SHIFT) | layer_id};
        return true;
    case VIA_ENCODER_ACTION_OSL:
        if (param & VIA_ENCODER_LAYER_PAYLOAD_MASK || !VIA_SL_BEHAVIOR[0]) return false;
        *binding = (struct zmk_behavior_binding){.behavior_dev = VIA_SL_BEHAVIOR,
                                                  .param1 = layer_id};
        return true;
    case VIA_ENCODER_ACTION_TT:
        if (param & VIA_ENCODER_LAYER_PAYLOAD_MASK) return false;
        *binding = (struct zmk_behavior_binding){
            .behavior_dev = VIA_COMPAT_BEHAVIOR,
            .param1 = (VIA_COMPAT_ACTION_TT << VIA_COMPAT_ACTION_SHIFT) | layer_id};
        return true;
    default:
        return false;
    }
}


static bool via_legacy_encoder_param_to_qmk(uint32_t param, uint16_t *keycode) {
    uint8_t basic;
    uint8_t qmk_mods;

    if (param == 0) {
        *keycode = QMK_KC_NO;
        return true;
    }

    if (!via_usage_to_qmk_basic(param, &basic) ||
        !via_zmk_mods_to_qmk(param >> 24, &qmk_mods)) {
        return false;
    }
    *keycode = ((uint16_t)qmk_mods << 8) | basic;
    return true;
}

static bool via_encoder_binding_to_qmk(const struct zmk_behavior_binding *binding,
                                       uint16_t *clockwise, uint16_t *counterclockwise) {
    if (!binding || !binding->behavior_dev) {
        return false;
    }

    if (strcmp(binding->behavior_dev, VIA_ENCODER_BEHAVIOR) == 0) {
        return via_encoder_param_to_qmk(binding->param1, clockwise) &&
               via_encoder_param_to_qmk(binding->param2, counterclockwise);
    }

    if (strcmp(binding->behavior_dev, VIA_INC_DEC_KP_BEHAVIOR) == 0) {
        return via_legacy_encoder_param_to_qmk(binding->param1, clockwise) &&
               via_legacy_encoder_param_to_qmk(binding->param2, counterclockwise);
    }

    return false;
}

static bool via_write_encoder(uint8_t layer, uint8_t encoder, bool clockwise,
                              uint16_t keycode) {
    if (!IS_ENABLED(CONFIG_ZMK_VIA_ENCODER)) {
        return false;
    }
    uint16_t raw_keycodes[2];
    uint32_t stored_keycodes[2];
    struct zmk_behavior_binding updated;

    if (layer >= ZMK_KEYMAP_LAYERS_LEN || encoder >= ZMK_KEYMAP_SENSORS_LEN ||
        !VIA_ENCODER_BEHAVIOR[0]) {
        return false;
    }

    const zmk_keymap_layer_id_t layer_id = zmk_keymap_layer_index_to_id(layer);
    struct zmk_behavior_binding current;
    if (zmk_keymap_get_sensor_binding_copy(layer_id, encoder, &current) < 0 ||
        !via_encoder_binding_to_qmk(&current, &raw_keycodes[0], &raw_keycodes[1])) {
        return false;
    }

    raw_keycodes[clockwise ? 0 : 1] = keycode;
    for (size_t i = 0; i < ARRAY_SIZE(raw_keycodes); i++) {
        struct zmk_behavior_binding translated;
        if (!via_make_binding(raw_keycodes[i], &translated) ||
            !via_normalize_encoder_keycode(raw_keycodes[i], &stored_keycodes[i])) {
            return false;
        }
    }

    updated = (struct zmk_behavior_binding){
        .behavior_dev = VIA_ENCODER_BEHAVIOR,
        .param1 = stored_keycodes[0],
        .param2 = stored_keycodes[1],
    };
    return zmk_keymap_set_sensor_binding_at_idx(layer_id, encoder, updated) >= 0;
}

static bool via_read_encoder(uint8_t layer, uint8_t encoder, bool clockwise,
                             uint16_t *keycode) {
    if (!IS_ENABLED(CONFIG_ZMK_VIA_ENCODER)) {
        return false;
    }
    uint16_t raw_keycodes[2];

    if (layer >= ZMK_KEYMAP_LAYERS_LEN || encoder >= ZMK_KEYMAP_SENSORS_LEN) {
        return false;
    }

    const zmk_keymap_layer_id_t layer_id = zmk_keymap_layer_index_to_id(layer);
    struct zmk_behavior_binding binding;
    if (zmk_keymap_get_sensor_binding_copy(layer_id, encoder, &binding) < 0 ||
        !via_encoder_binding_to_qmk(&binding, &raw_keycodes[0], &raw_keycodes[1])) {
        return false;
    }

    *keycode = raw_keycodes[clockwise ? 0 : 1];
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
    if (layer >= ZMK_KEYMAP_LAYERS_LEN || row >= VIA_KEYMAP_ROWS || column >= VIA_KEYMAP_COLS) {
        return false;
    }
    if (!via_slot_to_zmk_position(row, column, &position)) {
        *keycode = QMK_KC_NO;
        return true;
    }

    const zmk_keymap_layer_id_t layer_id = zmk_keymap_layer_index_to_id(layer);
    struct zmk_behavior_binding binding;
    if (zmk_keymap_get_layer_binding_copy(layer_id, position, &binding) < 0 ||
        !via_binding_to_qmk(&binding, keycode)) {
        *keycode = QMK_KC_NO;
    }
    return true;
}
static bool via_binding_is_unsupported(uint8_t layer, uint8_t position) {
    if (layer >= ZMK_KEYMAP_LAYERS_LEN) {
        return false;
    }
    const zmk_keymap_layer_id_t layer_id = zmk_keymap_layer_index_to_id(layer);
    struct zmk_behavior_binding binding;
    uint16_t keycode;
    return zmk_keymap_get_layer_binding_copy(layer_id, position, &binding) == 0 &&
           !via_binding_to_qmk(&binding, &keycode);
}

static bool via_make_binding(uint16_t keycode, struct zmk_behavior_binding *binding) {
    if (!binding || !via_qmk_to_binding(keycode, binding) || !binding->behavior_dev ||
        !binding->behavior_dev[0] ||
        !zmk_behavior_get_binding(binding->behavior_dev)) {
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

static struct sensor_value via_encoder_remainder[MAX(1, ZMK_KEYMAP_SENSORS_LEN)]
                                                        [MAX(1, ZMK_KEYMAP_LAYERS_LEN)];
static int via_encoder_triggers[MAX(1, ZMK_KEYMAP_SENSORS_LEN)]
                               [MAX(1, ZMK_KEYMAP_LAYERS_LEN)];

static int via_encoder_accept_data(
    struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event,
    const struct zmk_sensor_config *sensor_config, size_t channel_data_size,
    const struct zmk_sensor_channel_data channel_data[channel_data_size]) {
    ARG_UNUSED(binding);

    if (!sensor_config || !channel_data || channel_data_size == 0 ||
        sensor_config->triggers_per_rotation == 0 ||
        sensor_config->triggers_per_rotation > 360 || event.layer < 0 ||
        event.layer >= ZMK_KEYMAP_LAYERS_LEN || event.position < ZMK_KEYMAP_LEN) {
        return -EINVAL;
    }

    const uint32_t sensor_index =
        ZMK_SENSOR_POSITION_FROM_VIRTUAL_KEY_POSITION(event.position);
    if (sensor_index >= ZMK_KEYMAP_SENSORS_LEN) {
        return -EINVAL;
    }

    const struct sensor_value value = channel_data[0].value;
    int triggers;
    if (value.val1 == 0) {
        triggers = value.val2;
    } else {
        struct sensor_value remainder = via_encoder_remainder[sensor_index][event.layer];
        remainder.val1 += value.val1;
        remainder.val2 += value.val2;

        if (remainder.val2 >= 1000000 || remainder.val2 <= -1000000) {
            remainder.val1 += remainder.val2 / 1000000;
            remainder.val2 %= 1000000;
        }

        const int trigger_degrees = 360 / sensor_config->triggers_per_rotation;
        triggers = remainder.val1 / trigger_degrees;
        remainder.val1 %= trigger_degrees;
        via_encoder_remainder[sensor_index][event.layer] = remainder;
    }

    via_encoder_triggers[sensor_index][event.layer] = triggers;
    return 0;
}

static int via_encoder_process(struct zmk_behavior_binding *binding,
                               struct zmk_behavior_binding_event event,
                               enum behavior_sensor_binding_process_mode mode) {
    if (!binding || !binding->behavior_dev || event.layer < 0 ||
        event.layer >= ZMK_KEYMAP_LAYERS_LEN || event.position < ZMK_KEYMAP_LEN) {
        return ZMK_BEHAVIOR_TRANSPARENT;
    }

    const uint32_t sensor_index =
        ZMK_SENSOR_POSITION_FROM_VIRTUAL_KEY_POSITION(event.position);
    if (sensor_index >= ZMK_KEYMAP_SENSORS_LEN) {
        return ZMK_BEHAVIOR_TRANSPARENT;
    }

    int triggers = via_encoder_triggers[sensor_index][event.layer];
    if (mode != BEHAVIOR_SENSOR_BINDING_PROCESS_MODE_TRIGGER) {
        via_encoder_triggers[sensor_index][event.layer] = 0;
        return ZMK_BEHAVIOR_TRANSPARENT;
    }
    if (triggers == 0) {
        return ZMK_BEHAVIOR_TRANSPARENT;
    }

    const uint32_t raw_param = triggers > 0 ? binding->param1 : binding->param2;
    if (triggers < 0) {
        triggers = -triggers;
    }

    struct zmk_behavior_binding translated;
    bool valid = ((raw_param & VIA_ENCODER_LAYER_TAG_MASK) == VIA_ENCODER_LAYER_TAG)
                     ? via_encoder_tagged_param_to_binding(raw_param, &translated)
                     : via_encoder_param_to_binding(raw_param, &translated);
    if (!valid || !translated.behavior_dev ||
        !zmk_behavior_get_binding(translated.behavior_dev)) {
        via_encoder_triggers[sensor_index][event.layer] = 0;
        LOG_ERR("Rejecting invalid VIA encoder parameter 0x%08X", raw_param);
        return ZMK_BEHAVIOR_TRANSPARENT;
    }

#if IS_ENABLED(CONFIG_ZMK_SPLIT)
    event.source = ZMK_POSITION_STATE_CHANGE_SOURCE_LOCAL;
#endif

    for (int i = 0; i < triggers; i++) {
        int ret = zmk_behavior_queue_add_pair(&event, translated, 5);
        if (ret < 0) {
            LOG_ERR("VIA encoder queue exhausted (%d)", ret);
            via_encoder_triggers[sensor_index][event.layer] = 0;
            return ZMK_BEHAVIOR_TRANSPARENT;
        }
    }

    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api via_encoder_driver_api = {
    .sensor_binding_accept_data = via_encoder_accept_data,
    .sensor_binding_process = via_encoder_process,
};

#if IS_ENABLED(CONFIG_ZMK_VIA_ENCODER)
BEHAVIOR_DT_DEFINE(DT_NODELABEL(via_encoder), NULL, NULL, NULL, NULL, POST_KERNEL,
                   CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &via_encoder_driver_api);
#endif

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
            positions[i] = UINT8_MAX;
            continue;
        }
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

static bool via_get_macro_buffer(uint16_t offset, uint8_t size, uint8_t *data) {
    const size_t buffer_size = zmk_via_macro_get_buffer_size();
    if (size > VIA_REPORT_SIZE - 4 || (size_t)offset > buffer_size) {
        return false;
    }

    const size_t available = MIN((size_t)size, buffer_size - offset);
    if (zmk_via_macro_get_buffer(offset, available, data) < 0) {
        return false;
    }
    memset(data + available, 0, size - available);
    return true;
}

#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW)
static uint8_t via_rgb_speed_to_via(uint8_t speed) {
    if (speed < 1 || speed > 5) {
        return 0;
    }

    /* Use the five evenly spaced canonical values across the VIA range. */
    return ((uint16_t)(speed - 1) * UINT8_MAX + 2) / 4;
}

static uint8_t via_rgb_speed_from_via(uint8_t value) {
    /* Round to the nearest of the five ZMK speed levels. */
    return MIN(5, (uint8_t)(1 + (((uint16_t)value * 4 + 127) / UINT8_MAX)));
}

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
        report[3] = via_rgb_speed_to_via(zmk_rgb_underglow_get_speed());
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
    case VIA_RGB_EFFECT_SPEED:
        return zmk_rgb_underglow_set_speed(via_rgb_speed_from_via(report[3])) >= 0;
    case VIA_RGB_COLOR:
        color.h = ((uint32_t)report[3] * 360 + 127) / UINT8_MAX;
        color.s = ((uint16_t)report[4] * 100 + 127) / UINT8_MAX;
        return zmk_rgb_underglow_set_hsb(color) >= 0;
    default:
        return false;
    }
}
#endif
#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW)
static bool via_indication_restore_off;

static void via_indication_work_handler(struct k_work *work) {
    ARG_UNUSED(work);
    if (via_indication_restore_off) {
        (void)zmk_rgb_underglow_off();
        via_indication_restore_off = false;
    }
}

static K_WORK_DELAYABLE_DEFINE(via_indication_work, via_indication_work_handler);

static bool via_device_indicate(uint8_t value) {
    if (value == 0) {
        return zmk_rgb_underglow_off() >= 0;
    }
    bool was_on;
    if (zmk_rgb_underglow_get_state(&was_on) < 0 ||
        zmk_rgb_underglow_on() < 0 ||
        zmk_rgb_underglow_select_effect(0) < 0) {
        return false;
    }
    via_indication_restore_off = !was_on;
    k_work_reschedule(&via_indication_work, K_MSEC(250));
    return true;
}
#else
static bool via_device_indicate(uint8_t value) {
    ARG_UNUSED(value);
    return false;
}
#endif

static struct k_work_delayable via_bootloader_work;
static bool via_handle_report(uint8_t *report, bool *changed_out) {
    bool changed = false;
    bool handled = true;
    const uint8_t command = report[0];
    via_layer_order_snapshot_valid =
        zmk_keymap_get_layer_order_snapshot(&via_layer_order_snapshot) == 0;

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
            memset(&report[2], 0, MIN(28, VIA_REPORT_SIZE - 2));
            break;
        default:
            handled = false;
            break;
        }
        break;
    case VIA_CMD_SET_KEYBOARD_VALUE:
        handled = report[1] == VIA_VALUE_DEVICE_INDICATION &&
                  via_device_indicate(report[2]);
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
        report[1] = zmk_via_macro_get_count();
        break;
    case VIA_CMD_BOOTLOADER_JUMP:
        k_work_reschedule(&via_bootloader_work, K_MSEC(100));
        break;
    case VIA_CMD_MACRO_GET_BUFFER_SIZE: {
        const size_t size = zmk_via_macro_get_buffer_size();
        report[1] = size >> 8;
        report[2] = size;
        break;
    }
    case VIA_CMD_MACRO_GET_BUFFER:
        handled = via_get_macro_buffer(((uint16_t)report[1] << 8) | report[2], report[3],
                                       &report[4]);
        break;
    case VIA_CMD_MACRO_SET_BUFFER: {
        const int ret = report[3] <= VIA_REPORT_SIZE - 4
                            ? zmk_via_macro_set_buffer(
                                  ((uint16_t)report[1] << 8) | report[2], report[3], &report[4])
                            : -EINVAL;
        handled = ret >= 0;
        changed = ret > 0;
        break;
    }
    case VIA_CMD_MACRO_RESET: {
        const int ret = zmk_via_macro_reset();
        handled = ret >= 0;
        changed = ret > 0;
        break;
    }
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
        if (report[3] > 1) {
            handled = false;
            break;
        }
        changed = via_write_encoder(report[1], report[2], report[3] != 0,
                                    ((uint16_t)report[4] << 8) | report[5]);
        handled = changed;
        break;
    default:
        handled = false;
        break;
    }

    *changed_out = handled && changed;
    if (!handled) {
        report[0] = VIA_CMD_UNHANDLED;
    }
    via_layer_order_snapshot_valid = false;
    return handled;
}
static void via_bootloader_work_handler(struct k_work *work) {
    ARG_UNUSED(work);
    sys_reboot(RST_UF2);
}

static K_WORK_DELAYABLE_DEFINE(via_bootloader_work, via_bootloader_work_handler);

static struct k_work via_save_work;
static uint8_t response_report[VIA_REPORT_SIZE];

static void via_save_work_handler(struct k_work *work) {
    (void)work;
    int ret = zmk_keymap_save_changes();
    if (ret < 0) {
        LOG_ERR("Failed to persist VIA keymap changes: %d", ret);
    }

    ret = zmk_via_macro_save_changes();
    if (ret < 0) {
        LOG_ERR("Failed to persist VIA macro changes: %d", ret);
    }
}

static int via_raw_hid_received_listener(const zmk_event_t *event) {
    const struct raw_hid_received_event *received = as_raw_hid_received_event(event);
    if (!received || !received->data || received->length != VIA_REPORT_SIZE) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    memset(response_report, 0, sizeof(response_report));
    memcpy(response_report, received->data, VIA_REPORT_SIZE);

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
