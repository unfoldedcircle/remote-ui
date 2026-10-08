## Context

### Current State Analysis (commit `54d3cb8c`, branch `feat/button-simulator-repeat`)

- `main.qml:931` installs `InputController` as event filter on the application window
  (`ui.inputController.setSource(applicationWindow)`); the filter is `InputController::eventFilter()`
  (`src/ui/inputController.cpp:233`). It swallows input while `blockInput` is set (`:236-251`),
  then handles key presses (`:254`) and releases, including the 150 ms deferral of an
  auto-repeat-flagged release (`:296-317`), for every key in `m_keyCodeMapping`.
- BACK is `Qt::Key_Exit` (`src/ui/inputController.h:35`). The device keypad sends it from the
  kernel's `KEY_EXIT` (Qt evdev keymap, `qevdevkeyboard_defaultmap_p.h:669`); the button
  simulator sends it through `emitKey()` / `pressSimulatorKey()`, which build the event in
  `sendKeyEvent()` (`:108-111`).
- `Qt::Key_Escape` is not in the button map, and no QML or C++ code handles Escape or registers a
  shortcut. The `desktop-simulator` spec states that Escape acts as no button.
- 38 popup objects in `src/qml` (`Popup`, one `Drawer`, one `ComboBox`): 17 set `closePolicy: Popup.CloseOnPressOutside`, 19 set
  `Popup.NoAutoClose`, two keep Qt's default `CloseOnEscape | CloseOnPressOutside`:
  `components/LoadingScreen.qml:12` and the popup of the `ComboBox` in
  `components/integrations/fields/Dropdown.qml:70`. On the desktop these two close on Escape today.
- How Qt 5.15 closes a popup on Escape: a visible popup with `CloseOnEscape` grabs the
  `QKeySequence::Cancel` shortcut (`qquickpopup.cpp:2076-2079, 2462, 2634`). For every key press,
  `QGuiApplicationPrivate::processKeyEvent()` first calls
  `QWindowSystemInterface::handleShortcutEvent()` (`qguiapplication.cpp:2397`; on macOS the Cocoa
  plugin calls it, `qnsview_keys.mm:125`). That sends a `ShortcutOverride` key event, created
  ignored (`qevent.cpp:1103`), synchronously to the window (`qwindowsysteminterface.cpp:456-468`),
  where event filters see it before `QQuickWindow` forwards it to the focus item
  (`qquickwindow.cpp:1869-1872`). Only when nobody accepted it does the shortcut map run; a matched
  shortcut consumes the key press, which then never reaches the window.

### Constraints

- ADR 0007: a key travels two paths; the translation must give both the same key.
- ADR 0008 / ADR 0016: no change for the Remote Two and the Remote 3.
- ADR 0009: new logic gets a unit test. ADR 0012: the translation is C++, not QML.

## Goals / Non-Goals

**Goals:**

- On `DEV`, Escape behaves exactly like the BACK button on both input paths, held Escape included.
- On `DEV`, no Escape reaches a control; the two popups with the default close policy react to BACK
  instead.

**Non-Goals:**

- Mapping Backspace, the numeric keypad Enter or any other key.
- Translating Escape on `UCR2` / `UCR3` (a desktop run of a device model is unsupported, and a
  device has no Escape key).
- Changing the close policy of the two popups.

## Decisions

### D1 — Translate in `InputController::eventFilter()`, on `DEV` only

When `m_model` is `DEV` and the event is a key event for `Qt::Key_Escape`, the filter, after the
`blockInput` check:

- `ShortcutOverride`: accepts the event and returns `true`. The shortcut map is not consulted, so
  a popup's Escape shortcut does not fire, and Qt delivers the Escape press to the window.
- `KeyPress` / `KeyRelease`: sends a key event for `Qt::Key_Exit` of the same type and auto-repeat
  flag to the window through `sendKeyEvent()`, and returns `true` so the Escape event is dropped.
  The new event passes through the same filter, so the BACK press and release take every existing
  branch: owner routing, the 150 ms deferral of an auto-repeat-flagged release, the keypad-active
  state, and then the QML focus chain.

A blocked input (`blockInput(true)`) swallows Escape like every other key; the translated event
would be swallowed anyway.

- *Alternative: add `Qt::Key_Escape` → `BACK` to `m_keyCodeMapping`.* One line, but the focus
  chain would still receive Escape, not `Key_Exit`, the two popups would still close on it, and
  the mapping would apply to every model. Rejected.
- *Alternative: give the two popups an explicit `closePolicy` without `CloseOnEscape`.* Fixes the
  popups only and touches device screens for a desktop concern; a future popup with the default
  policy would bring the problem back. Rejected.
- *Alternative: an application-wide event filter.* Not needed: the `ShortcutOverride` reaches the
  window, where the filter already sits.

### D2 — The translated event is the simulator's BACK event

`sendKeyEvent()` builds the same event as the BACK button of the button simulator (no modifiers,
text from `QKeySequence`). The device's event carries no text; nothing in the UI reads the text of
a button event. Modifiers held with Escape are dropped, as the device's BACK has none. Like the
button simulator's events, the translated events are sent directly to the window and bypass the
shortcut map; the UI registers no shortcuts.

## Risks / Trade-offs

Failure Mode Analysis (input ownership / keyboard focus surface):

- [Escape translated on a device] → the branch requires `m_model == DEV`; device models keep the
  current code path. A unit test covers a non-`DEV` model.
- [Escape and BACK both handled] → the Escape press and release are dropped (`return true`).
- [A popup still closes on Escape] → the `ShortcutOverride` for Escape is accepted, so the
  `QKeySequence::Cancel` shortcut never fires; checked in the desktop walk with the loading screen
  or the integration dropdown, if reachable in the Remote-Core Simulator.
- [A text field relied on Escape] → no QML or C++ code handles Escape today; a device never sends it.
- [Held Escape differs from a held BACK] → the auto-repeat flag is kept; the desktop keyboard's
  release/press pairs take the existing 150 ms deferral, as for the arrow keys.
- [Endless recursion] → the translated event is `Key_Exit`, which the Escape branch does not match.

Trade-offs:

- Resource impact: one key code comparison per key event on `DEV`, one model comparison on the
  device. Negligible by inspection; no timers, no allocations.

## Migration Plan

- No data or configuration migration; rollback is a revert.
- Stacked on `feat/button-simulator-repeat` (reuses `sendKeyEvent()` and `testInputController`);
  the pull request targets that branch and is rebased onto `main` once it is merged.
- Verification target: unit tests for the event translation; desktop simulator (`UC_MODEL=DEV`)
  against the Remote-Core Simulator with the computer keyboard, because the change exists only
  there. Walk: Escape leaves a settings page; held Escape behaves like a held BACK; Backspace does
  nothing; the input log (`uc.ui.input` at debug level) shows BACK presses and releases and no
  Escape. No device run is needed: the device code path does not change.

## Open Questions

None.
