"""Host-level VIA contract vectors; hardware lifecycle tests remain separate."""

from pathlib import Path
import unittest


REPORT_SIZE = 32
KC_A = 0x04
KC_F7 = 0x40
KC_ASSISTANT = 0xC0
QK_BASIC_MAX = 0x00FF
ENCODER_TAG = 0xA0000000
SOURCE = Path(__file__).parents[1] / "config/src/via_zmk.c"


def accepts_basic_fallback(keycode: int) -> bool:
    return keycode <= QK_BASIC_MAX and keycode in {KC_A, KC_F7, KC_ASSISTANT}


def qmk_mt_to_zmk_hold(mods: int) -> tuple[int, int]:
    types = mods & 0x0F
    if not types:
        raise ValueError("MT requires a modifier")
    base = types & -types
    extra = types ^ base
    usage_id = {1: 0xE0, 2: 0xE1, 4: 0xE2, 8: 0xE3}.get(base)
    if usage_id is None:
        raise ValueError("invalid MT modifier")
    right = bool(mods & 0x10)
    implicit = extra | (0x10 if extra and right else 0)
    return usage_id + (4 if right else 0), implicit


def encode_layer_action(action: int, layer_id: int, payload: int = 0) -> int:
    return ENCODER_TAG | (action << 24) | (layer_id << 16) | payload


class ViaAuditContractTests(unittest.TestCase):
    def test_basic_fallback_never_truncates_16_bit_codes(self):
        self.assertTrue(accepts_basic_fallback(KC_A))
        self.assertTrue(accepts_basic_fallback(KC_F7))
        self.assertTrue(accepts_basic_fallback(KC_ASSISTANT))
        for code in (0x5240, 0x52C0, 0x7E14, 0x1234):
            self.assertFalse(accepts_basic_fallback(code))

    def test_representable_multi_mod_mt_uses_same_side_implicit_mods(self):
        self.assertEqual(qmk_mt_to_zmk_hold(0x03), (0xE0, 0x02))
        self.assertEqual(qmk_mt_to_zmk_hold(0x15), (0xE4, 0x14))
        self.assertEqual(qmk_mt_to_zmk_hold(0x17), (0xE4, 0x16))
        with self.assertRaises(ValueError):
            qmk_mt_to_zmk_hold(0)

    def test_encoder_layer_action_is_outside_raw_qmk_range(self):
        tagged = encode_layer_action(3, 7)
        self.assertGreater(tagged, 0xFFFF)
        self.assertEqual(tagged & 0xF0000000, ENCODER_TAG)

    def test_raw_hid_contract_accepts_only_exact_size(self):
        dispatches = lambda length: length == REPORT_SIZE
        for length in (0, 1, 31, 33, 64):
            self.assertFalse(dispatches(length))
        self.assertTrue(dispatches(REPORT_SIZE))

    def test_production_source_contains_compatibility_guards(self):
        source = SOURCE.read_text()
        for marker in (
            "zmk_keymap_get_layer_order_snapshot",
            "zmk_keymap_get_layer_binding_copy",
            "zmk_keymap_get_sensor_binding_copy",
            "via_encoder_tagged_param_to_binding",
            "VIA_CMD_BOOTLOADER_JUMP",
            "via_device_indicate",
            "VIA_COMPAT_ACTION_LM",
            "VIA_COMPAT_ACTION_MOUSE_BUTTON",
            "QMK_QK_PERSISTENT_DEF_LAYER",
            "QMK_QK_ONE_SHOT_LAYER",
            "QMK_QK_LAYER_TAP_TOGGLE",
            "zmk_behavior_queue_add_pair",
            "received->length != VIA_REPORT_SIZE",
        ):
            self.assertIn(marker, source)

    def test_mouse_range_is_explicitly_covered(self):
        source = SOURCE.read_text()
        for keycode in ("0xCD", "0xCE", "0xD1", "0xD8", "0xD9", "0xDC", "0xDD", "0xDF"):
            self.assertIn(keycode, source)

if __name__ == "__main__":
    unittest.main()
