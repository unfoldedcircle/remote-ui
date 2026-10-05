## Context

Current State Analysis, measured on `main` @ `1cc19491`:

- **`YIO1` in the code:** `enum Enum { DEV, YIO1, UCR2, UCR3 }` in `src/hardware/hardwareModel.h`;
  `src/ui/uiController.cpp` lists it in `getKeyNavigationEnabled()` (true, like every model) and
  not in `getShowRegulatoryInfo()` (false, like `DEV`); `src/hardware/hardwareController.cpp`
  creates the no-op `Haptic` and `TouchSlider` for it through the `default` branch, as for
  `DEV`; `src/main.cpp` parses `UC_MODEL` and falls back to `DEV` silently for an unknown value. Nothing else branches on it: `YIO1` behaves exactly like `DEV`.
- **Dead files:** thirteen layout directories under `src/qml/keyboard/layouts/` are listed in
  no `resources/qrc/keyboard.qrc` entry and referenced nowhere, so no build contains them: ten
  carry the "YIO-Remote software project" licence header (`ro_RO`, `en_GB`, `zh_TW`, `pt_BR`,
  `zh_CN`, `sl_SI`, `ru_RU`, `hr_HR`, `sk_SK`, `et_EE`), three do not (`bg_BG`, `cs_CZ`, `el_GR`). The 14 shipped languages have their adapted layouts embedded (change
  `add-missing-keyboard-layouts`).
- **The hardware layer:** `src/hardware/` is 2 099 lines, of which `wifi.*` (812), `power.*`
  (154), `battery.*` (176) and `info.*` (84) only talk to the core through the Core-API
  (`m_core->wifi…`, `getPowerMode`, `systemCommand`, `getSystemInfo`); the direct hardware code is
  `ucr2/hapticUCR2|UCR3` (sysfs write) and `ucr3/touchSliderUCR3` plus its event filter (evdev,
  ioctl), with no-op bases for the desktop. The naming is the residue of the YIO design
  (ADR 0016).
- **Docs:** `CLAUDE.md` ("`DEV` | `YIO1` | `UCR2` | `UCR3`"), `README.md` (`UC_MODEL`), and the
  living specs `hardware-platform`, `app-startup`, `desktop-simulator` list `YIO1`; the
  `platform-constraints` requirement of the open non-functional change already says it is not
  supported.

## Goals / Non-Goals

**Goals:** no `YIO1` anywhere; no dead YIO files; `src/hardware/` limited to real hardware
backends where the move is mechanical; docs and specs say what the code does.

**Non-Goals:** any behaviour change on a device or for `DEV`; redesigning the hardware
controller; touching the shipped keyboard layouts.

## Decisions

- **D1 — `YIO1` becomes an unknown value, not an alias of `DEV`.** Keeping an alias would keep the
  name alive in the enum and the docs; the fallback path for unknown values already gives the
  same result. That fallback is silent today; adding a warning would be a behaviour change and is
  not part of a removal (a possible small follow-up).
- **D2 — Delete the unembedded layouts rather than keep them "for later".** They are stock Qt
  Virtual Keyboard layouts with a foreign licence header, retrievable from Qt at any time; the
  adapted layouts the remote ships are a different artefact (`add-missing-keyboard-layouts`).
- **D3 — Re-home the Core-API clients as a file move only.** The four classes moved to
  `src/system/`; namespace `uc::hw`, the logging categories (which users filter on) and the QML
  singleton names stay, and the hardware controller still creates them. Renaming the namespace
  and moving the object creation out of the controller are a follow-up, not a removal.

## Risks / Trade-offs

- [Somebody still sets `UC_MODEL=YIO1` in a script] → runs as `DEV`, silently, as for any
  unknown value; documented in the changelog.
- [`resources/translations/en_US.ts` still names `src/hardware/{battery,wifi}.cpp` in its
  `<location>` tags] → the translation contexts are unchanged, so nothing breaks; the next
  `lupdate` refresh rewrites the paths.
- [A test target compiles a moved file] → the test CMake lists are edited with the move and
  `make test` proves it.

## Migration Plan

Merge the implementation branch; no data, no core change. Archive this change afterwards.
Rollback is reverting the commit.

## Open Questions

None.
