Phases 1 and 3 are file-disjoint and can run in parallel. Phase 2 depends on phase 1 (it calls the
new `InputController` invokables). Phase 4 runs last, on the combined result. One commit per phase;
the `CHANGELOG.md` entry goes into the phase 2 commit, which makes the change visible.

## 1. Repeat timing in InputController (C++)

- [x] 1.1 In `src/ui/inputController.h/.cpp`, factor the `QKeyEvent` construction and
      `sendEvent` of `emitKey()` into a private helper that takes the event type, the key and the
      auto-repeat flag; `emitKey()` keeps its signature and behaviour.
- [x] 1.2 Add the repeat delay (600 ms) and period (150 ms) as named constants, a `QTimer` member
      and the held key, and connect the timeout to send a press with the auto-repeat flag and
      restart the timer at the period.
- [x] 1.3 Add `Q_INVOKABLE void pressSimulatorKey(Qt::Key key)`: release a different key that is
      still held, send a plain press, start the timer at the delay.
- [x] 1.4 Add `Q_INVOKABLE void releaseSimulatorKey(Qt::Key key)`: if `key` is the held key, stop
      the timer, clear the held key and send a plain release; otherwise do nothing.
- [x] 1.5 Add `test/ui/test_input_controller.cpp`: install `InputController` (model `DEV`) on a
      plain `QObject` source and record `keyPressed` / `keyReleased`, plus an event filter that
      records each `QKeyEvent`'s type and `isAutoRepeat()`. Cases: short hold (release before
      600 ms → one press, one release, no auto-repeat); long hold (~1 s → one plain press, then
      auto-repeat presses, the first not before 600 ms, then one plain release delivered at once,
      not after the 150 ms deferral); no repeat after the release; `releaseSimulatorKey` for a key
      that is not held sends nothing; `emitKey()` still sends exactly one plain event.
- [x] 1.6 Register the test in `test/ui/CMakeLists.txt` as `testInputController`: sources
      `test_input_controller.cpp`, `../../src/ui/inputController.cpp`, `../../src/logging.cpp`,
      and for AUTOMOC the header-only `../../src/core/enums.h` (`Q_GADGET`) and
      `../../src/hardware/hardwareModel.h` (`Q_OBJECT`); link `Qt5::Quick`, which the file already
      finds for `testEntityController`; `add_test`.

## 2. Simulator button (QML, depends on 1.3 and 1.4)

- [x] 2.1 In `src/qml/button-simulator/Button.qml`, call `ui.inputController.pressSimulatorKey(key)`
      in `onPressed` and `releaseSimulatorKey(key)` in `onReleased` and `onCanceled`.
- [x] 2.2 Release a held key when the `Button` is destroyed while pressed
      (`Component.onDestruction`; unconditional, since `releaseSimulatorKey` ignores a key that is
      not held), for the window's `Loader` unloading the content when the window is hidden.
- [x] 2.3 `CHANGELOG.md`, "Unreleased", "Changed": holding a button in the desktop button
      simulator now auto-repeats like the device keypad (after 600 ms, every 150 ms), so repeat
      handlers can be tried without the computer keyboard.

## 3. Documentation

- [x] 3.1 `docs/key-navigation.md` §8: replace "The button simulator sends one press and one
      release per click, no auto-repeat; check repeat handlers with a held key of the computer
      keyboard or on a device." with the device-like repeat (600 ms, then every 150 ms) and that
      the computer keyboard repeats with the desktop's own delay and rate.

## 4. Verification

- [x] 4.1 `make test` (or `ctest -R testInputController` in `test/build`) passes.
- [x] 4.2 Format the changed lines (`git add -u && git clang-format && git add -u`) and run
      `./cpplint.sh`.
- [x] 4.3 `make linux`, then `git checkout resources/translations/` and leave the generated `.qm`
      files uncommitted.
- [x] 4.4 Desktop walk with `UC_MODEL=DEV` against the Remote-Core Simulator, `uc.ui.input` at
      debug level: hold DPAD_DOWN on a list (selection keeps moving); short-click DPAD_DOWN (one
      step); hold a key with a `long_press` handler (long press once, no short press); hold POWER
      3 s (power off menu); press an area, drag off and release (release sent, repeats stop);
      cancel a held press and hide the simulator window during a hold (release sent, repeats
      stop). The Remote-Core Simulator configuration has no pages and no activities, so the main
      screen owns no input: the long press is walked with DPAD_MIDDLE on the profile switcher
      instead of HOME on a page, and VOLUME_UP on an activity is not walked; both take the same
      press path as the walked keys.
- [x] 4.5 Run `openspec validate button-simulator-auto-repeat`; the `desktop-simulator` delta is
      synced into the living spec when the change is archived.
