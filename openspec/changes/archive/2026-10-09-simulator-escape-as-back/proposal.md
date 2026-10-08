## Why

On the desktop simulator (`UC_MODEL=DEV`) the computer keyboard covers the d-pad, HOME, VOICE and
MENU, but not BACK: BACK has to be clicked in the button simulator window, so walking a screen with
the keyboard means switching between keyboard and mouse at every step back. Escape is the
natural key for it, and today it does nothing useful: no screen handles it, and the two popups
that keep Qt's default close policy (the loading screen and the ComboBox popup of the integration
setup dropdown) close on it, which never happens on a device.

## What Changes

- On `DEV`, the Escape key of the computer keyboard acts as BACK. The UI turns every Escape key
  event into the key event of the BACK button (`Qt::Key_Exit`), keeping press or release and the
  auto-repeat flag, and drops the Escape event. Both input paths, the button-navigation handlers
  and the QML focus chain, therefore see what the BACK button of a device sends.
- On `DEV`, no Escape reaches a control: a popup whose close policy includes closing on Escape
  no longer closes on it, but reacts to BACK as on a device.
- Backspace and the numeric keypad Enter stay unmapped. On the Remote Two and the Remote 3 nothing
  changes.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `desktop-simulator`: "Desktop keyboard as keypad" maps Escape to BACK on `DEV` instead of
  excluding it; its "Escape" scenario changes accordingly.

## Impact

- Hardware models: none. The translation is limited to the desktop model `DEV`; the Remote Two
  and the Remote 3 binaries take the same code path as today.
- Code: `InputController::eventFilter()` (`src/ui/inputController.cpp`). No new files.
- Tests: `test/ui/test_input_controller.cpp`, which this change extends.
- Stacked on the button simulator auto-repeat change (`button-simulator-auto-repeat`, branch
  `feat/button-simulator-repeat`): it reuses the key event helper and the `testInputController`
  target that change adds.
- Core-API: none. Third-party code and assets: none.
- Docs: `docs/key-navigation.md` §8 (desktop testing), `CHANGELOG.md`.
