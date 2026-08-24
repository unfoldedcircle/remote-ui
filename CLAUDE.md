# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Qt 5.15 / QML UI for the Unfolded Circle Remote Two/Three. C++17, `CONFIG += c++17`.
**Qt 5 only** — do not use Qt 6 APIs, `QtQuick` 6 imports, or `qt_add_qml_module`. The `find_package(QT NAMES Qt6 Qt5 ...)` in the test CMake files is misleading; every target links `Qt5::`.

Two independent build systems: **qmake** (`remote-ui.pro`) builds the app; **CMake** (`test/`) builds *only* the unit tests. Neither can do the other's job.

## Registering new files (easy to get wrong)

- `remote-ui.pro` lists every header and source **explicitly** — no wildcards. A new `src/**/*.cpp` / `*.h` must be added to both `HEADERS` and `SOURCES` or it silently isn't compiled.
- Nothing is loaded from disk at runtime; the app loads `qrc:/main.qml`. Every new `.qml` must be added to the matching file in `resources/qrc/` (`main.qrc` for app QML, plus `button-simulator.qrc`, `keyboard.qrc`, `icons.qrc`, `images.qrc`, `translations.qrc`). A missing entry fails only at runtime with "file not found". `main.qrc` uses `alias=` with paths relative back to `../../src/qml/...`.
- Tests don't link a library — each `test/*/CMakeLists.txt` re-compiles the `../../src/*.cpp` files it needs, and must also list header-only files containing `Q_OBJECT`/`Q_GADGET` so AUTOMOC picks them up. Adding a dependency to code under test means editing that CMakeLists source list.

## File header (required on every new file, C++ and QML)

```
// Copyright (c) {year} Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later
```

License is GPL-3.0-or-later.

## Build

```bash
mkdir build && cd build
qmake ../remote-ui.pro CONFIG+=release   # add CONFIG+=static for embedded
make -j $(nproc --all)
```

**Running qmake rewrites the working tree.** The TRANSLATION section of `remote-ui.pro` shells out to `lupdate`/`lrelease` at qmake time, rewriting every tracked `resources/translations/*.ts` and generating untracked `.qm` files. Never commit that churn — `git checkout resources/translations/` after a build. If `lupdate`/`lrelease` aren't on PATH, qmake only warns and the build then fails on missing `.qm`.

Device builds are aarch64 static via docker only (`docs/cross-compile.md`); there is no in-repo toolchain file.

## Test

See the `/test` skill. Short form:

```bash
mkdir -p test/build && cd test/build
cmake -D CMAKE_PREFIX_PATH="$QT_ROOT_DIR" .. && cmake --build .
ctest            # or: ctest -R testCore for one target
```

QtTest framework. Targets: `testCommon`, `testCore`, `testHardware`, `testBattery`, `testUiModels`. All four test CMakeLists force `CMAKE_OSX_ARCHITECTURES x86_64` (old Qt has no arm64 build), so on Apple Silicon this needs an x86_64 Qt under Rosetta. `tests.bak/` is stale — ignore it.

## Style

`.clang-format` (Google base, but `IndentWidth: 4`, `ColumnLimit: 120`, `AccessModifierOffset: -3`, `AlignConsecutiveDeclarations: true`). A PostToolUse hook runs `clang-format -i` on edited C++.

Lint must pass CI (`.github/workflows/code_guidelines.yml`): `./cpplint.sh`. CI also fails if any source file name contains a space.

Conventions in existing code: `#pragma once` (not include guards); members `m_`, statics `s_`; namespaces `uc::`, `uc::ui::`, `uc::hw::`, `uc::core::`, `uc::integration::`, `uc::dock::`. **Logging only through the categories in `src/logging.h`** — `qCDebug(lcApp())` etc., never raw `qDebug()`.

QML has no linter or formatter configured; match surrounding style.

## Desktop simulator

Desktop == simulator, selected by `UC_MODEL` (`DEV` | `YIO1` | `UCR2` | `UCR3`; unset/invalid → `DEV`). `DEV` opens a scaled 480x850 window plus a second window from `src/qml/button-simulator/` emulating the physical buttons. Hardware paths (haptic, touch slider, wifi, power) branch on the model in `src/hardware/hardwareController.cpp` and do not run on desktop.

Needs the [core-simulator](https://github.com/unfoldedcircle/core-simulator) running via docker-compose. Key env vars — `UC_TOKEN_PATH` (required, point at `$CORE_SIMULATOR_PATH/docker/ui-env/ws-token`), `UC_SOCKET_URL` (default `ws://127.0.0.1:8080/ws`), `UC_DISPLAY_WIDTH`/`_HEIGHT`/`_SCALE`, `UC_RESOURCE_PATH`, `UC_LEGAL_PATH`, `UC_SOUND_EFFECTS_PATH`, `UC_ONBOARDING_PATH`. Full list in `README.md` and `remote-ui.pro`.

### Keyboard focus in the DEV simulator (read before testing key navigation)

`DEV` opens a second window, the button simulator. A second window takes the window manager focus, and then the main window has **no active focus item at all** — `applicationWindow.activeFocusItem` stays `null`, and even `forceActiveFocus()` on a bare `Item` in `main.qml` leaves `activeFocus == false`. On the device `ui.showRegulatoryInfo` is true, the button simulator is never created, and focus behaves normally.

Consequence: every settings page that navigates through the QML focus chain (`KeyNavigation.up`/`down` plus `highlight: activeFocus`) looks completely dead in `DEV` — no highlight, no reaction to the d-pad — while working on hardware. Do not conclude such a page is broken from a `DEV` run.

`main.qml` sets `Qt.WindowDoesNotAcceptFocus` on the button simulator window to keep `DEV` behaving like the device. If you add another `Window`, do the same. To check key navigation independently of that, run single-window: `UC_MODEL=UCR2`.

### The two input paths

Every key press travels two paths, and neither cancels the other:

1. `InputController::eventFilter`, installed on the window, never consumes the event. It emits `keyPressedFor(owner, key)`, which drives the `Components.ButtonNavigation` config of whichever scope currently holds `takeControl()`.
2. The same event continues into the normal QML focus chain — `KeyNavigation`, `Keys.on*`, `ListView` arrow handling, `Slider` left/right.

A page must therefore commit to **one** idiom: either a `ListView` driven by `ButtonNavigation`, or a focus chain. Adding a `DPAD_UP`/`DOWN` handler to a page that also has a `KeyNavigation` chain makes one key press do both things. `Keys` handlers and `event.accepted` only stop QML-side propagation; they cannot suppress the `ButtonNavigation` path.

Use `buttonNavigation.extendDefaultConfig({...})`, never `buttonNavigation.defaultConfig: {...}` — assigning replaces the whole object and silently drops the `BACK` / `HOME` handlers `Settings.Page` declares, leaving the page impossible to exit with the keypad.

### Which layer the keys belong to

`takeControl()` only redirects path 1. It does **not** move the keyboard focus, so a page keeps its focus — and keeps reacting to `KeyNavigation` and `Keys` handlers — while a popup is open on top of it. One key press then acts on two layers at once.

`Components.ButtonNavigation` has an opt-in `manageFocus` that ties the focus to the input ownership: the focus is parked on the (inert) `ButtonNavigation` item while another layer owns the input, and handed back to the control the user was on when the scope returns to the front. `Settings.Page` sets it, so every settings page is covered; a popup only needs it if the popup itself navigates by focus (`WifiInfo`, `WifiJoin`). Set `initialFocusItem` for the control that should be focused on first entry.

Two things to know when working on this: a swipe view hands the focus to its page wrapper *after* a level change, so `ButtonNavigation` re-claims the focus whenever it owns the input but the focus sits outside its scope; and `overrideActive: true` bypasses ownership entirely — a handler under it fires for *any* owner, so avoid it for anything a popup can cover.

`Settings.Page` exposes `scrollTarget`: point it at the page's `Flickable` and the focused control is kept on screen. `Flickable` does not follow the keyboard focus on its own; `ListView` does scroll to its `currentIndex` by itself.

## Repo etiquette

- Conventional commits: `feat:`, `fix:`, `docs:`, `chore:`. Squash-merged PRs get a `(#NNN)` suffix.
- Branches: `fix/<issue-number>`, `feat/<slug>`.
- **Update `CHANGELOG.md`** for every user-visible fix or feature — Keep a Changelog format, `## vX.Y.Z - YYYY-MM-DD` heading with `### Added` / `### Fixed`, written as user-facing prose, not commit-speak. Conventionally a separate `docs: update changelog` commit.
- Version comes from `git describe --match "v[0-9]*" --tags` at qmake time, not from a file. `vX.Y.Z` tags trigger a release; pushes to `main` produce a `latest` prerelease.
- Run `git submodule update --init --recursive` — `3rd-party/QR-Code-generator` is the only submodule.

## Translations

`resources/translations/en_US.ts` is the only source of truth and the only translation file to ever edit or commit. All other languages are owned by SimpleLocalize, downloaded to the `l10n` branch and PR'd into `main`. `.qm` files are generated and never committed. `.gitignore` excludes `resources/translations/*` except `en_US.ts`, so other `.ts` files only got in via `git add -f` — don't add more.

Note: `resources/qrc/translations.qrc` currently embeds fewer languages than `remote-ui.pro` lists in `TRANSLATIONS`, and `src/translation/translation.cpp` discovers languages by iterating `:/translations` — a language missing from the qrc is not loadable at runtime.

## Not part of the project

Untracked scratch that should not be edited or treated as source: `doc/` (note: tracked real docs are `docs/`), root-level `en_US.ts` / `en_US2.ts`, `3rd-party/vosk-api/`, `tests.bak/`, `tmp.bak/`, `build/`, `binaries/`, `.idea/`, `*.pro.user`.
