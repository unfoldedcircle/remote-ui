## Why

Holding a button in the desktop button simulator (`UC_MODEL=DEV`) sends one key press and, at the
end, one release. A device keypad auto-repeats a held key, so every `pressed_repeat` handler, every
repeat-driven list scroll and every "held value change" control is dead when it is tried with the
simulator. Developers have to switch to the computer keyboard, whose repeat comes as
release/press pairs at the desktop's own delay and rate, unlike the device. The
`desktop-simulator` spec already names simulator auto-repeat as a possible improvement.

## What Changes

- Holding a button in the simulator sends what a held device key sends: one press when the mouse
  button goes down, then, while it stays down, presses flagged as auto-repeat after 600 ms and every
  150 ms after that, and one plain release when the mouse button goes up. These are the
  auto-repeat delay and rate the firmware sets for the device keypad (`platform-constraints`).
- Every emulated button repeats, as on the device keypad, including HOME, BACK and POWER. Long-press
  and the 3 s POWER hold behave as before, because `key-navigation` ignores auto-repeat presses
  while a long press is pending.
- When the window system cancels a press (the mouse grab is lost), the simulator stops repeating
  and sends the release, so a held key can never keep repeating after the mouse button is up.
- The spec's statement that the simulator does not repeat, and the matching notes in the
  verification limits and in `docs/key-navigation.md`, are replaced.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `desktop-simulator`: "Press and hold in the button simulator is limited" becomes a requirement
  for device-like auto-repeat; "Emulated buttons and their key events" gains the cancelled-press
  release; "Desktop verification limits" drops "without auto-repeat" from the keypad limit.

## Impact

- Hardware models: none. Only the desktop model `DEV` shows the button simulator; on the Remote Two
  and the Remote 3 the window is never created, and the device input path is unchanged.
- Code: `InputController` gains a press and a release entry point for the simulator, which own the
  repeat timing; the simulator button (`src/qml/button-simulator/Button.qml`) calls them.
  `InputController::emitKey()` is unchanged. One new unit test target, no new app files.
- Core-API: none. Key presses reach the core only through the existing entity commands.
- Third-party code and assets: none.
- Docs: `docs/key-navigation.md` §8 (desktop testing), `CHANGELOG.md`.
