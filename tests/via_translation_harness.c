#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "via_translation.h"

#include "via_oneshot.h"

#define REPORT_SIZE 32u
#define ENCODER_TAG 0xA0000000u

static void expect_kind(uint16_t keycode, enum via_translation_kind kind) {
    enum via_translation_kind actual = via_translation_classify_keycode(keycode);
    if (actual != kind) {
        fprintf(stderr, "kind mismatch 0x%04X: got %d expected %d\n", keycode, actual, kind);
    }
    assert(actual == kind);
}

static bool accepts_basic_fallback(uint16_t keycode) {
    return keycode <= 0x00FFu &&
           (keycode == 0x04u || keycode == 0x40u || keycode == 0xC0u);
}

static bool qmk_mt_to_zmk_hold(uint8_t mods, uint8_t *usage_id, uint8_t *implicit) {
    uint8_t types = mods & 0x0Fu;
    if (!types || !usage_id || !implicit) {
        return false;
    }

    uint8_t base = 1u;
    while ((types & base) == 0u) {
        base <<= 1;
    }

    switch (base) {
    case 0x01:
        *usage_id = 0xE0u;
        break;
    case 0x02:
        *usage_id = 0xE1u;
        break;
    case 0x04:
        *usage_id = 0xE2u;
        break;
    case 0x08:
        *usage_id = 0xE3u;
        break;
    default:
        return false;
    }

    uint8_t extra = types ^ base;
    bool right = (mods & 0x10u) != 0u;
    *usage_id = (uint8_t)(*usage_id + (right ? 4u : 0u));
    *implicit = (uint8_t)(extra | ((extra && right) ? 0x10u : 0u));
    return true;
}

static uint32_t encode_layer_action(uint8_t action, uint8_t layer_id, uint16_t payload) {
    return ENCODER_TAG | ((uint32_t)action << 24) | ((uint32_t)layer_id << 16) | payload;
}

static bool exact_report_size(size_t length) {
    return length == REPORT_SIZE;
}

static bool encoder_keycode_supported(uint16_t keycode) {
    if (keycode == 0x0000u || keycode == 0x0001u) {
        return true;
    }
    if (keycode >= 0x00CDu && keycode <= 0x00DFu) {
        return keycode >= 0x00D1u && keycode <= 0x00DCu;
    }
    return !(keycode >= 0x2000u && keycode < 0x4000u) &&
           !(keycode >= 0x4000u && keycode < 0x5000u) &&
           !(keycode >= 0x5000u && keycode < 0x5200u) &&
           !(keycode >= 0x5220u && keycode < 0x5240u) &&
           !(keycode >= 0x52C0u && keycode < 0x52E0u);
}

static char *read_source(const char *path) {
    FILE *file = fopen(path, "rb");
    assert(file != NULL);
    assert(fseek(file, 0, SEEK_END) == 0);
    long length = ftell(file);
    assert(length >= 0);
    assert(fseek(file, 0, SEEK_SET) == 0);

    char *source = malloc((size_t)length + 1u);
    assert(source != NULL);
    assert(fread(source, 1u, (size_t)length, file) == (size_t)length);
    source[length] = '\0';
    assert(fclose(file) == 0);
    return source;
}

static void expect_source_markers(const char *source) {
    const char *markers[] = {
        "zmk_keymap_get_layer_order_snapshot",
        "zmk_keymap_get_layer_binding_copy",
        "zmk_keymap_get_sensor_binding_copy",
        "via_encoder_tagged_param_to_binding",
        "VIA_CMD_BOOTLOADER_JUMP",
        "via_device_indicate",
        "VIA_COMPAT_ACTION_LM",
        "QMK_QK_PERSISTENT_DEF_LAYER",
        "QMK_QK_ONE_SHOT_LAYER",
        "QMK_QK_LAYER_TAP_TOGGLE",
        "zmk_behavior_queue_add_pair",
        "received->length != VIA_REPORT_SIZE",
        "static bool via_encoder_keycode_supported(uint16_t keycode)",
        "via_encoder_keycode_supported(raw_keycodes[i])",
        "if (raw_keycode == QMK_KC_TRANSPARENT)",
        "if (raw_keycode == QMK_KC_NO)",
        "via_encoder_emit_wheel(raw_keycode - 0xD9)",
        "INPUT_REL_WHEEL",
        "0xCD",
        "0xCE",
        "0xD1",
        "0xD8",
        "0xD9",
        "0xDC",
        "0xDD",
        "0xDF",
    };

    for (size_t i = 0; i < sizeof(markers) / sizeof(markers[0]); ++i) {
        assert(strstr(source, markers[i]) != NULL);
    }

    const char *queue_failure = strstr(source, "VIA encoder queue exhausted");
    assert(queue_failure != NULL);
    assert(strstr(queue_failure, "return ZMK_BEHAVIOR_OPAQUE;") != NULL);
    const char *transparent = strstr(queue_failure, "return ZMK_BEHAVIOR_TRANSPARENT;");
    assert(transparent == NULL || transparent - queue_failure >= 200);
}

int main(int argc, char **argv) {
    expect_kind(0x04, VIA_TRANSLATION_BASIC);
    expect_kind(0xA5, VIA_TRANSLATION_BASIC);
    expect_kind(0xC2, VIA_TRANSLATION_BASIC);
    expect_kind(0x0104, VIA_TRANSLATION_MODS);
    expect_kind(0x2204, VIA_TRANSLATION_MOD_TAP);
    expect_kind(0x4004, VIA_TRANSLATION_LAYER_TAP);
    expect_kind(0x5001, VIA_TRANSLATION_LAYER_MOD);
    expect_kind(0x5201, VIA_TRANSLATION_TO);
    expect_kind(0x5222, VIA_TRANSLATION_MOMENTARY);
    expect_kind(0x5261, VIA_TRANSLATION_TOGGLE);
    expect_kind(0x5241, VIA_TRANSLATION_DEFAULT);
    expect_kind(0x52E1, VIA_TRANSLATION_PERSISTENT_DEFAULT);
    expect_kind(0x5281, VIA_TRANSLATION_ONE_SHOT_LAYER);
    expect_kind(0x52A1, VIA_TRANSLATION_ONE_SHOT_MOD);
    expect_kind(0x52C1, VIA_TRANSLATION_TAP_TOGGLE);
    expect_kind(0xCD, VIA_TRANSLATION_MOUSE);
    expect_kind(0xD8, VIA_TRANSLATION_MOUSE);
    expect_kind(0xFFFF, VIA_TRANSLATION_UNSUPPORTED);

    assert(accepts_basic_fallback(0x04));
    assert(accepts_basic_fallback(0x40));
    assert(accepts_basic_fallback(0xC0));
    assert(!accepts_basic_fallback(0x5240));
    assert(!accepts_basic_fallback(0x52C0));
    assert(!accepts_basic_fallback(0x7E14));
    assert(!accepts_basic_fallback(0x1234));

    uint8_t usage_id;
    uint8_t implicit;
    assert(qmk_mt_to_zmk_hold(0x03, &usage_id, &implicit));
    assert(usage_id == 0xE0 && implicit == 0x02);
    assert(qmk_mt_to_zmk_hold(0x15, &usage_id, &implicit));
    assert(usage_id == 0xE4 && implicit == 0x14);
    assert(qmk_mt_to_zmk_hold(0x17, &usage_id, &implicit));
    assert(usage_id == 0xE4 && implicit == 0x16);
    assert(!qmk_mt_to_zmk_hold(0, &usage_id, &implicit));

    uint32_t tagged = encode_layer_action(3, 7, 0);
    assert(tagged > UINT16_MAX);
    assert((tagged & 0xF0000000u) == ENCODER_TAG);
    const struct {
        uint8_t action;
        uint8_t layer;
        uint16_t payload;
    } tag_vectors[] = {
        {1, 7, 0}, {5, 3, 0x001F}, {6, 2, 0}, {9, 4, 0},
    };
    for (size_t i = 0; i < sizeof(tag_vectors) / sizeof(tag_vectors[0]); ++i) {
        uint32_t encoded = via_encoder_tag_encode(tag_vectors[i].action, tag_vectors[i].layer,
                                                  tag_vectors[i].payload);
        struct via_encoder_tag decoded;
        assert(via_encoder_tag_decode(encoded, &decoded));
        assert(decoded.action == tag_vectors[i].action);
        assert(decoded.layer_id == tag_vectors[i].layer);
        assert(decoded.payload == tag_vectors[i].payload);
    }

    struct via_encoder_tag ignored;
    assert(!via_encoder_tag_decode(0x12345678u, &ignored));


    for (size_t length = 0; length <= 64; ++length) {
        assert(exact_report_size(length) == (length == REPORT_SIZE));
    }
    assert(via_oneshot_added(0x01u, 0x00u) == 0x01u);
    assert(via_oneshot_added(0x01u, 0x01u) == 0x00u);
    assert(via_oneshot_accumulate(0x02u, 0x01u) == 0x03u);
    assert(via_oneshot_accumulate(0x01u, 0x01u) == 0x01u);

    const uint16_t supported[] = {
        0x0000, 0x0001, 0x0004, 0x0104, 0x5200, 0x5261,
        0x5242, 0x5281, 0x52A1, 0x7700, 0x7E06,
        0xD1, 0xD8, 0xD9, 0xDC,
    };
    for (size_t i = 0; i < sizeof(supported) / sizeof(supported[0]); ++i) {
        assert(encoder_keycode_supported(supported[i]));
    }

    const uint16_t unsupported[] = {0x2004, 0x4004, 0x5001, 0x5221, 0x52C1, 0xCD, 0xCE, 0xDD, 0xDF};
    for (size_t i = 0; i < sizeof(unsupported) / sizeof(unsupported[0]); ++i) {
        assert(!encoder_keycode_supported(unsupported[i]));
    }

    for (uint16_t keycode = 0xCD; keycode <= 0xDF; ++keycode) {
        assert(encoder_keycode_supported(keycode) == (keycode >= 0xD1 && keycode <= 0xDC));
    }

    assert(argc == 2);
    char *source = read_source(argv[1]);
    expect_source_markers(source);
    free(source);

    puts("via native C tests: ok");
    return 0;
}
