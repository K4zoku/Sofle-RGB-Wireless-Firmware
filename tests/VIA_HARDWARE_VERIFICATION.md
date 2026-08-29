# VIA Hardware Verification

These cases require a flashed central board and stock VIA; the repository has no
native simulator for Raw HID, ZMK Studio, or flash persistence.

1. Flash `build/sofle_left/zephyr/zmk.uf2` and connect the central board.
2. In stock VIA, confirm protocol version, read/write keycodes, macros, RGB,
   and encoder tabs still work.
3. Send Raw HID reports of lengths 0, 1, 31, 32, and 33 bytes. Only 32 bytes
   may produce a reply or mutate state.
4. Assign encoder CW=`MO(1)` and CCW=`TG(2)`; record the logical layer IDs.
   Reorder layers in ZMK Studio, then verify GET_ENCODER reports the new
   indices while rotation still targets the recorded IDs. Repeat with LT and
   TO, then reboot and repeat to verify persistence.
5. Issue rapid key and encoder SET requests while a save is pending, then
   reset/discard. After reboot, verify flash matches the final runtime state.
6. Read native modified-tap bindings containing an implicit tap modifier;
   they must appear as `KC_NO`, and writing `KC_NO` must preserve the native
   binding.
