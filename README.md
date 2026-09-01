# ZMK VIA Stack

This repository provides the VIA compatibility module, Raw HID integration,
and the pinned ZMK and zmk-raw-hid dependencies.

## Repository layout

- `config/` is the west manifest and the VIA Zephyr module. Keep its module
  files in place.
- `user-config/` is where the complete consumer ZMK configuration directory
  belongs. It is intentionally empty and contains only `.gitkeep` by default.
- `zmk/` and `zmk-raw-hid/` are pinned git submodules.
- `config/patches/` contains the tracked changes applied to those submodules.

## Add a keyboard configuration

Clone the repository with its submodules:

```sh
git clone --recurse-submodules <repository-url> my-keyboard
cd my-keyboard
```

Copy the consumer configuration into `user-config/`. Do not replace the
repository `config/` directory.

```sh
cp /path/to/keyboard-config/*.conf user-config/
cp /path/to/keyboard-config/*.keymap user-config/
if [ -d /path/to/keyboard-config/boards ]; then
  cp -a /path/to/keyboard-config/boards user-config/
fi
if [ -d /path/to/keyboard-config/dts ]; then
  cp -a /path/to/keyboard-config/dts user-config/
fi
```

A normal ZMK configuration can use the usual board, shield, and `default`
file names. If automatic keymap selection does not find the desired file,
pass it explicitly with `-DKEYMAP_FILE`.

The VIA keymap must provide a `zmk,via-matrix` node with `rows`, `columns`,
and `map`. The consumer configuration also needs `CONFIG_ZMK_VIA=y` and the
other Kconfig options required by the selected keyboard.

## Apply the VIA changes

Run west first, then apply the tracked changes to the clean submodules:

```sh
if [ ! -d .west ]; then
  west init -l config
fi
west update
bash scripts/apply-patches.sh
west zephyr-export
```

## Build firmware

Build with the board and shield for the selected keyboard. This example uses
the ZMK build container:

```sh
podman run --rm \
  -v "$PWD:/workspace" \
  -w /workspace \
  docker.io/zmkfirmware/zmk-build-arm:stable \
  bash -lc '\
    if [ ! -d /workspace/.west ]; then west init -l /workspace/config; fi && \
    west update && \
    bash /workspace/scripts/apply-patches.sh && \
    west zephyr-export && \
    west build -p always \
      -s /workspace/zmk/app \
      -b <board> \
      -d /workspace/build/firmware \
      -- \
      -DSHIELD="<shield>" \
      -DZMK_CONFIG=/workspace/user-config \
      -DKEYMAP_FILE=/workspace/user-config/<keymap>.keymap'
```

The firmware output is written to `build/firmware/zephyr/zmk.uf2`.

A VIA JSON definition is only needed by the VIA desktop application. It is
not required for the firmware build.

## Run native tests

The host contract tests use Meson, Ninja, and the system C compiler:

```sh
meson setup builddir
meson test -C builddir --print-errorlogs
```

## License

Original work in this repository is licensed under Apache-2.0 OR MIT, at your
option. See `LICENSE-APACHE` and `LICENSE-MIT`.

The ZMK, zmk-raw-hid, and west dependencies remain subject to their own
licenses and notices.
