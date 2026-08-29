#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "via_translation.h"

static void expect_kind(uint16_t keycode, enum via_translation_kind kind) {
    enum via_translation_kind actual = via_translation_classify_keycode(keycode);
    if (actual != kind) {
        fprintf(stderr, "kind mismatch 0x%04X: got %d expected %d\n", keycode, actual, kind);
    }
    assert(actual == kind);
}

int main(void) {
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
    expect_kind(0x1234, VIA_TRANSLATION_UNSUPPORTED);

    const struct {
        uint8_t action;
        uint8_t layer;
        uint16_t payload;
    } vectors[] = {
        {1, 7, 0}, {5, 3, 0x001F}, {6, 2, 0}, {9, 4, 0},
    };
    for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); ++i) {
        uint32_t encoded = via_encoder_tag_encode(vectors[i].action, vectors[i].layer,
                                                  vectors[i].payload);
        struct via_encoder_tag decoded;
        assert(via_encoder_tag_decode(encoded, &decoded));
        assert(decoded.action == vectors[i].action);
        assert(decoded.layer_id == vectors[i].layer);
        assert(decoded.payload == vectors[i].payload);
    }

    struct via_encoder_tag ignored;
    assert(!via_encoder_tag_decode(0x12345678u, &ignored));
    puts("via translation vectors: ok");
    return 0;
}
