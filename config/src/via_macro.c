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

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
static const struct behavior_parameter_value_metadata via_macro_param_values[] = {{
    .display_name = "Macro ID",
    .range = {
        .min = 0,
        .max = CONFIG_ZMK_VIA_MACRO_COUNT - 1,
    },
    .type = BEHAVIOR_PARAMETER_VALUE_TYPE_RANGE,
}};

static const struct behavior_parameter_metadata_set via_macro_metadata_set = {
    .param1_values = via_macro_param_values,
    .param1_values_len = ARRAY_SIZE(via_macro_param_values),
};

static const struct behavior_parameter_metadata via_macro_metadata = {
    .sets_len = 1,
    .sets = &via_macro_metadata_set,
};
#endif

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
    if (ret < 0) {
        k_mutex_unlock(&via_macro_mutex);
        return ret;
    }
    if ((size_t)ret != len) {
        LOG_ERR("Short VIA macro setting read: %d/%d", ret, len);
        k_mutex_unlock(&via_macro_mutex);
        return -EIO;
    }
    via_macro_dirty = false;
    k_mutex_unlock(&via_macro_mutex);
    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(via_macro, "via_macro", NULL, via_macro_handle_set, NULL, NULL);

#define VIA_MACRO_ACTION_PREFIX 0x01
#define VIA_MACRO_ACTION_TAP 0x01
#define VIA_MACRO_ACTION_DOWN 0x02
#define VIA_MACRO_ACTION_UP 0x03
#define VIA_MACRO_ACTION_DELAY 0x04
#define VIA_MACRO_DELAY_TERMINATOR '|'

enum via_macro_action_kind {
    VIA_MACRO_ACTION_END,
    VIA_MACRO_ACTION_CHARACTER,
    VIA_MACRO_ACTION_KEY_TAP,
    VIA_MACRO_ACTION_KEY_DOWN,
    VIA_MACRO_ACTION_KEY_UP,
    VIA_MACRO_ACTION_WAIT,
};

struct via_macro_action {
    enum via_macro_action_kind kind;
    uint8_t keycode;
    uint32_t delay_ms;
    size_t next_offset;
};

static bool via_macro_find_start_locked(uint8_t id, size_t *offset) {
    size_t current = 0;

    for (uint8_t macro = 0; macro < id; macro++) {
        while (current < ARRAY_SIZE(via_macro_buffer) && via_macro_buffer[current] != 0) {
            current++;
        }
        if (current == ARRAY_SIZE(via_macro_buffer)) {
            return false;
        }
        current++;
    }

    *offset = current;
    return true;
}

static bool via_macro_parse_action_locked(size_t offset, struct via_macro_action *action) {
    const size_t buffer_size = ARRAY_SIZE(via_macro_buffer);

    if (offset >= buffer_size) {
        action->kind = VIA_MACRO_ACTION_END;
        action->next_offset = offset;
        return true;
    }

    const uint8_t byte = via_macro_buffer[offset];
    if (byte == 0) {
        action->kind = VIA_MACRO_ACTION_END;
        action->next_offset = offset + 1;
        return true;
    }

    if (byte == VIA_MACRO_ACTION_PREFIX && offset + 1 < buffer_size) {
        const uint8_t type = via_macro_buffer[offset + 1];
        if ((type == VIA_MACRO_ACTION_TAP || type == VIA_MACRO_ACTION_DOWN ||
             type == VIA_MACRO_ACTION_UP) &&
            offset + 2 < buffer_size) {
            action->kind = type == VIA_MACRO_ACTION_TAP
                               ? VIA_MACRO_ACTION_KEY_TAP
                               : (type == VIA_MACRO_ACTION_DOWN ? VIA_MACRO_ACTION_KEY_DOWN
                                                                : VIA_MACRO_ACTION_KEY_UP);
            action->keycode = via_macro_buffer[offset + 2];
            action->next_offset = offset + 3;
            return true;
        }

        if (type == VIA_MACRO_ACTION_DELAY) {
            size_t cursor = offset + 2;
            uint32_t delay = 0;
            bool has_digits = false;

            while (cursor < buffer_size && via_macro_buffer[cursor] !=
                                                VIA_MACRO_DELAY_TERMINATOR) {
                const uint8_t digit = via_macro_buffer[cursor];
                if (digit < '0' || digit > '9' ||
                    delay > (UINT32_MAX - (digit - '0')) / 10) {
                    break;
                }
                delay = delay * 10 + (digit - '0');
                has_digits = true;
                cursor++;
            }

            if (has_digits && cursor < buffer_size &&
                via_macro_buffer[cursor] == VIA_MACRO_DELAY_TERMINATOR) {
                action->kind = VIA_MACRO_ACTION_WAIT;
                action->delay_ms = delay;
                action->next_offset = cursor + 1;
                return true;
            }
        }
    }

    action->kind = VIA_MACRO_ACTION_CHARACTER;
    action->keycode = byte;
    action->next_offset = offset + 1;
    return true;
}

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

static bool via_macro_qmk_to_binding(uint8_t keycode, struct zmk_behavior_binding *binding) {
    uint32_t usage;

    if (!VIA_MACRO_KP_BEHAVIOR[0] || !via_qmk_basic_to_usage(keycode, &usage)) {
        return false;
    }

    *binding = (struct zmk_behavior_binding){
        .behavior_dev = VIA_MACRO_KP_BEHAVIOR,
        .param1 = usage,
    };
    return true;
}

struct via_macro_executor_state {
    struct zmk_behavior_binding_event event;
    struct zmk_behavior_binding pending_binding;
    size_t offset;
    bool active;
    bool pending_release;
};

static struct via_macro_executor_state via_macro_executor;

static void via_macro_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(via_macro_work, via_macro_work_handler);

static void via_macro_work_handler(struct k_work *work) {
    ARG_UNUSED(work);

    struct via_macro_action action = {0};
    struct zmk_behavior_binding binding = {0};
    struct zmk_behavior_binding_event event = {0};
    bool invoke = false;
    bool pressed = false;
    uint32_t wait_ms = 0;

    k_mutex_lock(&via_macro_mutex, K_FOREVER);
    if (!via_macro_executor.active) {
        k_mutex_unlock(&via_macro_mutex);
        return;
    }

    if (via_macro_executor.pending_release) {
        binding = via_macro_executor.pending_binding;
        event = via_macro_executor.event;
        via_macro_executor.pending_release = false;
        invoke = true;
        wait_ms = CONFIG_ZMK_VIA_MACRO_DELAY_MS;
    } else if (via_macro_buffer[ARRAY_SIZE(via_macro_buffer) - 1] != 0 ||
               !via_macro_parse_action_locked(via_macro_executor.offset, &action)) {
        via_macro_executor.active = false;
    } else {
        via_macro_executor.offset = action.next_offset;

        switch (action.kind) {
        case VIA_MACRO_ACTION_END:
            via_macro_executor.active = false;
            break;
        case VIA_MACRO_ACTION_WAIT:
            wait_ms = action.delay_ms;
            break;
        case VIA_MACRO_ACTION_KEY_TAP:
            invoke = via_macro_qmk_to_binding(action.keycode, &binding);
            pressed = true;
            if (invoke) {
                via_macro_executor.pending_binding = binding;
                via_macro_executor.pending_release = true;
            }
            wait_ms = CONFIG_ZMK_VIA_MACRO_DELAY_MS;
            break;
        case VIA_MACRO_ACTION_KEY_DOWN:
        case VIA_MACRO_ACTION_KEY_UP:
            invoke = via_macro_qmk_to_binding(action.keycode, &binding);
            pressed = action.kind == VIA_MACRO_ACTION_KEY_DOWN;
            wait_ms = CONFIG_ZMK_VIA_MACRO_DELAY_MS;
            break;
        case VIA_MACRO_ACTION_CHARACTER:
            invoke = via_macro_char_to_binding(action.keycode, &binding);
            pressed = true;
            if (invoke) {
                via_macro_executor.pending_binding = binding;
                via_macro_executor.pending_release = true;
            }
            wait_ms = CONFIG_ZMK_VIA_MACRO_DELAY_MS;
            break;
        }
    }

    const bool schedule_next = via_macro_executor.active;
    event = via_macro_executor.event;
    event.timestamp = k_uptime_get();
    k_mutex_unlock(&via_macro_mutex);

    if (invoke) {
        const int ret = zmk_behavior_invoke_binding(&binding, event, pressed);
        if (ret < 0) {
            LOG_ERR("Failed to invoke VIA macro key action: %d", ret);
        }
    }

    if (schedule_next) {
        (void)k_work_schedule(&via_macro_work, K_MSEC(wait_ms));
    }
}

int zmk_via_macro_enqueue(uint8_t id, const struct zmk_behavior_binding_event *event) {
    if (!event || id >= CONFIG_ZMK_VIA_MACRO_COUNT) {
        return -EINVAL;
    }

    k_mutex_lock(&via_macro_mutex, K_FOREVER);
    if (via_macro_executor.active) {
        k_mutex_unlock(&via_macro_mutex);
        return -EBUSY;
    }

    /* A non-zero final byte marks a VIA transfer that was not completed. */
    if (via_macro_buffer[ARRAY_SIZE(via_macro_buffer) - 1] != 0 ||
        !via_macro_find_start_locked(id, &via_macro_executor.offset)) {
        k_mutex_unlock(&via_macro_mutex);
        return 0;
    }

    via_macro_executor.event = *event;
    via_macro_executor.pending_release = false;
    via_macro_executor.active = true;
    k_mutex_unlock(&via_macro_mutex);

    const int ret = k_work_schedule(&via_macro_work, K_NO_WAIT);
    return ret < 0 ? ret : 0;
}

static int on_via_macro_binding_pressed(struct zmk_behavior_binding *binding,
                                        struct zmk_behavior_binding_event event) {
    const int ret = zmk_via_macro_enqueue((uint8_t)binding->param1, &event);
    if (ret < 0) {
        LOG_ERR("Failed to start VIA macro %u: %d", (unsigned int)binding->param1, ret);
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
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .parameter_metadata = &via_macro_metadata,
#endif
};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &via_macro_driver_api);
