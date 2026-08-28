#define DT_DRV_COMPAT zmk_via_macro

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/util.h>

#include <dt-bindings/zmk/hid_usage_pages.h>
#include <dt-bindings/zmk/modifiers.h>
#include <drivers/behavior.h>
#include <zmk/behavior_queue.h>

#include <via_macro.h>

LOG_MODULE_REGISTER(zmk_via_macro, CONFIG_ZMK_LOG_LEVEL);

#define VIA_MACRO_SETTINGS_KEY "via_macro/buffer"

#if DT_NODE_EXISTS(DT_NODELABEL(kp))
#define VIA_MACRO_KP_BEHAVIOR DEVICE_DT_NAME(DT_NODELABEL(kp))
#else
#define VIA_MACRO_KP_BEHAVIOR ""
#endif

BUILD_ASSERT(CONFIG_ZMK_VIA_MACRO_BUFFER_SIZE <= UINT16_MAX,
             "VIA macro buffer must fit the VIA 16-bit offset");

static uint8_t via_macro_buffer[CONFIG_ZMK_VIA_MACRO_BUFFER_SIZE];
static bool via_macro_dirty;
K_MUTEX_DEFINE(via_macro_mutex);

int zmk_via_macro_get_count(void) {
    return CONFIG_ZMK_VIA_MACRO_COUNT;
}

size_t zmk_via_macro_get_buffer_size(void) {
    return ARRAY_SIZE(via_macro_buffer);
}

int zmk_via_macro_get_buffer(uint16_t offset, uint8_t size, uint8_t *data) {
    if (!data || (size_t)offset > ARRAY_SIZE(via_macro_buffer) ||
        (size_t)size > ARRAY_SIZE(via_macro_buffer) - offset) {
        return -EINVAL;
    }

    k_mutex_lock(&via_macro_mutex, K_FOREVER);
    memcpy(data, &via_macro_buffer[offset], size);
    k_mutex_unlock(&via_macro_mutex);
    return 0;
}

int zmk_via_macro_set_buffer(uint16_t offset, uint8_t size, const uint8_t *data) {
    if (!data || (size_t)offset > ARRAY_SIZE(via_macro_buffer) ||
        (size_t)size > ARRAY_SIZE(via_macro_buffer) - offset) {
        return -EINVAL;
    }

    k_mutex_lock(&via_macro_mutex, K_FOREVER);
    const bool changed = memcmp(&via_macro_buffer[offset], data, size) != 0;
    if (changed) {
        memcpy(&via_macro_buffer[offset], data, size);
        via_macro_dirty = true;
    }
    k_mutex_unlock(&via_macro_mutex);
    return changed ? 1 : 0;
}

int zmk_via_macro_reset(void) {
    k_mutex_lock(&via_macro_mutex, K_FOREVER);
    bool changed = false;
    for (size_t i = 0; i < ARRAY_SIZE(via_macro_buffer); i++) {
        if (via_macro_buffer[i] != 0) {
            changed = true;
            break;
        }
    }
    if (changed) {
        memset(via_macro_buffer, 0, sizeof(via_macro_buffer));
        via_macro_dirty = true;
    }
    k_mutex_unlock(&via_macro_mutex);
    return changed ? 1 : 0;
}

int zmk_via_macro_save_changes(void) {
    k_mutex_lock(&via_macro_mutex, K_FOREVER);
    if (!via_macro_dirty) {
        k_mutex_unlock(&via_macro_mutex);
        return 0;
    }

    /* VIA uses a non-zero last byte while the host is rewriting the buffer. */
    if (via_macro_buffer[ARRAY_SIZE(via_macro_buffer) - 1] != 0) {
        k_mutex_unlock(&via_macro_mutex);
        return 0;
    }

    const int ret = settings_save_one(VIA_MACRO_SETTINGS_KEY, via_macro_buffer,
                                      sizeof(via_macro_buffer));
    if (ret >= 0) {
        via_macro_dirty = false;
    }
    k_mutex_unlock(&via_macro_mutex);
    return ret;
}

static int via_macro_handle_set(const char *name, size_t len, settings_read_cb read_cb,
                                void *cb_arg) {
    const char *next;
    if (!settings_name_steq(name, "buffer", &next) || next) {
        return 0;
    }
    if (len > sizeof(via_macro_buffer)) {
        LOG_ERR("VIA macro setting is too large: %d", len);
        return -EINVAL;
    }

    k_mutex_lock(&via_macro_mutex, K_FOREVER);
    memset(via_macro_buffer, 0, sizeof(via_macro_buffer));
    const int ret = read_cb(cb_arg, via_macro_buffer, len);
    if (ret >= 0) {
        via_macro_dirty = false;
    }
    k_mutex_unlock(&via_macro_mutex);
    return ret;
}

SETTINGS_STATIC_HANDLER_DEFINE(via_macro, "via_macro", NULL, via_macro_handle_set, NULL, NULL);

static bool via_macro_char_to_binding(uint8_t c, struct zmk_behavior_binding *binding) {
    uint8_t usage_id;
    uint8_t modifiers = 0;

    if (!VIA_MACRO_KP_BEHAVIOR[0]) {
        return false;
    }

    if (c >= 'a' && c <= 'z') {
        usage_id = 0x04 + (c - 'a');
    } else if (c >= 'A' && c <= 'Z') {
        usage_id = 0x04 + (c - 'A');
        modifiers = MOD_LSFT;
    } else if (c >= '1' && c <= '9') {
        usage_id = 0x1E + (c - '1');
    } else if (c == '0') {
        usage_id = 0x27;
    } else {
        switch (c) {
        case ' ':
            usage_id = 0x2C;
            break;
        case '\t':
            usage_id = 0x2B;
            break;
        case '\n':
        case '\r':
            usage_id = 0x28;
            break;
        case '\b':
        case 0x7F:
            usage_id = 0x2A;
            break;
        case 0x1B:
            usage_id = 0x29;
            break;
        case '!':
            usage_id = 0x1E;
            modifiers = MOD_LSFT;
            break;
        case '@':
            usage_id = 0x1F;
            modifiers = MOD_LSFT;
            break;
        case '#':
            usage_id = 0x20;
            modifiers = MOD_LSFT;
            break;
        case '$':
            usage_id = 0x21;
            modifiers = MOD_LSFT;
            break;
        case '%':
            usage_id = 0x22;
            modifiers = MOD_LSFT;
            break;
        case '^':
            usage_id = 0x23;
            modifiers = MOD_LSFT;
            break;
        case '&':
            usage_id = 0x24;
            modifiers = MOD_LSFT;
            break;
        case '*':
            usage_id = 0x25;
            modifiers = MOD_LSFT;
            break;
        case '(':
            usage_id = 0x26;
            modifiers = MOD_LSFT;
            break;
        case ')':
            usage_id = 0x27;
            modifiers = MOD_LSFT;
            break;
        case '-':
            usage_id = 0x2D;
            break;
        case '_':
            usage_id = 0x2D;
            modifiers = MOD_LSFT;
            break;
        case '=':
            usage_id = 0x2E;
            break;
        case '+':
            usage_id = 0x2E;
            modifiers = MOD_LSFT;
            break;
        case '[':
            usage_id = 0x2F;
            break;
        case '{':
            usage_id = 0x2F;
            modifiers = MOD_LSFT;
            break;
        case ']':
            usage_id = 0x30;
            break;
        case '}':
            usage_id = 0x30;
            modifiers = MOD_LSFT;
            break;
        case '\\':
            usage_id = 0x31;
            break;
        case '|':
            usage_id = 0x31;
            modifiers = MOD_LSFT;
            break;
        case ';':
            usage_id = 0x33;
            break;
        case ':':
            usage_id = 0x33;
            modifiers = MOD_LSFT;
            break;
        case '\'':
            usage_id = 0x34;
            break;
        case '"':
            usage_id = 0x34;
            modifiers = MOD_LSFT;
            break;
        case '`':
            usage_id = 0x35;
            break;
        case '~':
            usage_id = 0x35;
            modifiers = MOD_LSFT;
            break;
        case ',':
            usage_id = 0x36;
            break;
        case '<':
            usage_id = 0x36;
            modifiers = MOD_LSFT;
            break;
        case '.':
            usage_id = 0x37;
            break;
        case '>':
            usage_id = 0x37;
            modifiers = MOD_LSFT;
            break;
        case '/':
            usage_id = 0x38;
            break;
        case '?':
            usage_id = 0x38;
            modifiers = MOD_LSFT;
            break;
        default:
            return false;
        }
    }

    *binding = (struct zmk_behavior_binding){
        .behavior_dev = VIA_MACRO_KP_BEHAVIOR,
        .param1 = ((uint32_t)modifiers << 24) | ZMK_HID_USAGE(HID_USAGE_KEY, usage_id),
    };
    return true;
}

int zmk_via_macro_enqueue(uint8_t id, const struct zmk_behavior_binding_event *event) {
    if (!event || id >= CONFIG_ZMK_VIA_MACRO_COUNT) {
        return -EINVAL;
    }

    int ret = 0;
    uint16_t offset = 0;
    k_mutex_lock(&via_macro_mutex, K_FOREVER);

    /* A non-zero final byte marks a VIA transfer that was not completed. */
    if (via_macro_buffer[ARRAY_SIZE(via_macro_buffer) - 1] != 0) {
        goto out;
    }

    for (uint8_t macro = 0; macro < id; macro++) {
        while (offset < ARRAY_SIZE(via_macro_buffer) && via_macro_buffer[offset] != 0) {
            offset++;
        }
        if (offset == ARRAY_SIZE(via_macro_buffer)) {
            goto out;
        }
        offset++;
    }

    while (offset < ARRAY_SIZE(via_macro_buffer) && via_macro_buffer[offset] != 0) {
        struct zmk_behavior_binding binding;
        if (via_macro_char_to_binding(via_macro_buffer[offset], &binding)) {
            ret = zmk_behavior_queue_add(event, binding, true, CONFIG_ZMK_VIA_MACRO_DELAY_MS);
            if (ret < 0) {
                break;
            }
            ret = zmk_behavior_queue_add(event, binding, false, CONFIG_ZMK_VIA_MACRO_DELAY_MS);
            if (ret < 0) {
                break;
            }
        }
        offset++;
    }

out:
    k_mutex_unlock(&via_macro_mutex);
    return ret;
}

static int on_via_macro_binding_pressed(struct zmk_behavior_binding *binding,
                                        struct zmk_behavior_binding_event event) {
    const int ret = zmk_via_macro_enqueue((uint8_t)binding->param1, &event);
    if (ret < 0) {
        LOG_ERR("Failed to queue VIA macro %u: %d", (unsigned int)binding->param1, ret);
    }
    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_via_macro_binding_released(struct zmk_behavior_binding *binding,
                                         struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api via_macro_driver_api = {
    .binding_pressed = on_via_macro_binding_pressed,
    .binding_released = on_via_macro_binding_released,
};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &via_macro_driver_api);
