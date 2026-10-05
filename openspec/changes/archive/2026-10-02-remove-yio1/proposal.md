## Why

The first YIO remote is no longer supported (ADR 0008), but its model still exists in the code:
`YIO1` in the hardware model enumeration, switch arms that treat it like the desktop, and ten
keyboard layout files carrying the YIO-Remote licence header that are not embedded in any build.
The hardware layer itself dates from that remote, which had no core process; on the Remote Two
and Remote 3 the core owns the hardware and the UI drives only the haptic motor, input and sound
(ADR 0016), yet the Core-API clients for WiFi, power, battery and system information still sit
under `src/hardware/`. **The implementation is on the branch `chore/remove-yio1` (pull request
open).**

## What Changes

- **`YIO1` is removed** from `UC_MODEL`. The value is now an unrecognised one and starts the
  desktop simulator (`DEV`), like any other unknown value; every switch arm and document that
  named it goes, including the commented-out YIO entry of the build matrix.
- **Dead keyboard layouts are deleted:** the thirteen layout directories under
  `src/qml/keyboard/layouts/` that no build embeds (`resources/qrc/keyboard.qrc` does not list
  them; the keyboard only loads from `qrc:/keyboard/layouts`) — the ten with the YIO-Remote header
  plus `bg_BG`, `cs_CZ` and `el_GR`. Stock layouts come back from Qt Virtual Keyboard when a
  language is added.
- **The Core-API clients move out of `src/hardware/`:** `Wifi`, `Power`, `Battery` and `Info`
  now live in `src/system/`, as a file move only — namespace `uc::hw`, the `uc.hw.*` logging
  categories and the QML singletons (`Wifi`, `Power`, `Battery`, `HwInfo`) are unchanged, and the
  hardware controller still creates them. `src/hardware/` holds only the haptic and touch slider
  backends, the model and the controller.
- `CHANGELOG.md` records the removal for developers who set `UC_MODEL`.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `hardware-platform`: model selection, the regulatory-information flag and the key-navigation
  availability no longer list `YIO1`.
- `app-startup`: the `UC_MODEL` values.
- `desktop-simulator`: the button simulator window is a `DEV` feature only.

## Impact

- **Hardware models:** none on a device. On a desktop, `UC_MODEL=YIO1` now runs as `DEV`, which
  is what `YIO1` effectively was; the fallback is silent, as for every unknown value.
- **remote-core dependency:** none.
- **Code:** `src/hardware/hardwareModel.h`, `hardwareController.h`, `src/ui/uiController.cpp`,
  the deleted layout directories, `.github/workflows/build.yml`, `README.md`, `CLAUDE.md`,
  `CHANGELOG.md`; the moved files under `src/system/` with their `remote-ui.pro` and
  `test/hardware/CMakeLists.txt` registrations.
