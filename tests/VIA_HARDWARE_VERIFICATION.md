# VIA Hardware Verification

These cases require a flashed central board and stock VIA; the repository has no
native simulator for Raw HID, ZMK Studio, or flash persistence.

1. Flash `build/sofle_left/zephyr/zmk.uf2` and connect the central board.
2. In stock VIA, confirm protocol version, read/write keycodes, macros, RGB,
   and encoder tabs still work.
3. Send Raw HID reports of lengths 0, 1, 31, 32, and 33 bytes. Only 32 bytes
   may produce a reply or mutate state.
4. Assign encoder CW=`TG(2)` and CCW=`TO(1)`; record the logical layer IDs.
   Reorder layers in ZMK Studio, then verify GET_ENCODER reports the new
   indices while rotation still targets the recorded IDs. Reboot and repeat
   to verify persistence.
5. Set the active layer's encoder binding to `KC_TRNS` with a lower-layer
   action, then verify one detent executes the lower-layer action. Set it to
   `KC_NO`, then verify one detent consumes the event without executing the
   lower-layer action.
6. Assign each wheel direction (`KC_WH_U`, `KC_WH_D`, `KC_WH_L`, `KC_WH_R`)
   and verify one detent emits exactly one wheel step. Repeat with slow and
   rapid detents; stopping must not continue scrolling.
7. Verify SET_ENCODER rejects `MO`, `LM`, `LT`, `MT`, `TT`, mouse movement, and
   mouse acceleration keycodes. Existing stored unsupported actions must remain
   readable and act as an opaque no-op.
8. Issue rapid key and encoder SET requests while a save is pending, then
   reset/discard. After reboot, verify flash matches the final runtime state.
9. Read native modified-tap bindings containing an implicit tap modifier;
   they must appear as `KC_NO`, and writing `KC_NO` must preserve the native
   binding.
