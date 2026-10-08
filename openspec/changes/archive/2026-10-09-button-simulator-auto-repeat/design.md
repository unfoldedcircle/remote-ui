## Context

### Current State Analysis (commit `48c4ebdf`, release 0.83.0)

- `src/qml/button-simulator/Buttons.qml` lays out 21 `Button` instances, one per emulated key.
- `src/qml/button-simulator/Button.qml` (34 lines) has one `MouseArea` (`:28-33`):
  `onPressed` calls `ui.inputController.emitKey(key)`, `onReleased` calls
  `emitKey(key, true)`. There is no `onCanceled` handler, so a cancelled press sends no release.
- `InputController::emitKey()` (`src/ui/inputController.h:67`, `src/ui/inputController.cpp:69-73`)
  builds a `QKeyEvent` with `autorep = false` and sends it synchronously to the main window
  (`m_source`). It has one caller in the code base, `Button.qml`.
- `InputController::eventFilter()` uses the auto-repeat flag in two places:
  - press (`:228-246`): a plain press stores the current owner in `m_keyOwner[key]`, an
    auto-repeat press reuses the stored owner; a plain POWER press starts the 3 s hold timer.
  - release (`:259-274`): an auto-repeat-flagged release is deferred by 150 ms (the desktop
    keyboard's release/press pairs); a plain release is delivered at once.
- `ButtonNavigation.qml` (`:478-509`) does not read the flag: it treats every press that follows a
  press without a release in between as a repeat (`repeats[key]`), runs `pressed_repeat` or
  `pressed` for it, and ignores it while a `long_press` timer is pending.
- Device sequence, from Qt 5.15.19 `qevdevkeyboardhandler.cpp:252-255, 484`: the kernel reports
  a held key as one press, auto-repeat events (value 2, mapped to a press with
  `autoRepeat = true`), and one plain release. No release is reported in between. The firmware
  sets the kernel repeat to 600 ms delay and 150 ms period (`platform-constraints`).

### Constraints

- ADR 0012: QML is presentation only. The repeat timing decides *what* is sent and when, so it is
  logic and belongs in C++. `Button.qml` only forwards the mouse press and release.
- ADR 0009: new logic gets a unit test.
- ADR 0016 and the `desktop-simulator` spec: the button simulator exists only for `DEV`. The
  device input path must not change.

## Goals / Non-Goals

**Goals:**

- A held simulator button sends the device sequence: press, auto-repeat presses after 600 ms and
  then every 150 ms, plain release.
- A cancelled press ends the hold with a release.
- The timing logic is unit-tested without a display.

**Non-Goals:**

- Changing how `InputController` or `ButtonNavigation` treat repeats on the device.
- Emulating the Remote 3 keys (STOP, RECORD, MENU) or a Remote 3 keypad picture.
- Making the delay or rate configurable (no request for it).
- Changing the desktop keyboard path, whose repeat comes from the desktop's own settings.

## Decisions

### D1 — The repeat timer lives in `InputController`, behind two new invokables

`InputController` gets `Q_INVOKABLE void pressSimulatorKey(Qt::Key key)` and
`Q_INVOKABLE void releaseSimulatorKey(Qt::Key key)`, a single-shot-restarted `QTimer`
and the key being held. `pressSimulatorKey` sends a plain press and starts the timer at 600 ms;
each timeout sends a press with the auto-repeat flag and restarts the timer at 150 ms.
`releaseSimulatorKey` stops the timer and sends a plain release. A mouse can hold only one area at
a time, so one timer is enough. A press while another key is still held (cannot happen with one
mouse, but cheap to guard) first releases the held key.

`emitKey()` stays unchanged for scripted walks (`docs/key-navigation.md` §8). The event is built
in one private helper that both entry points use, which takes the auto-repeat flag.

- *Alternative: a QML `Timer` in `Button.qml`.* Fewer lines, but it puts timing logic in QML
  against ADR 0012, and it cannot be tested with the existing QtTest setup.
- *Alternative: a separate C++ class for the simulator.* Cleaner separation, but a new file pair,
  registration in `remote-ui.pro` and a new context property for about 30 lines of code that need
  `emitKey`'s private target anyway. Not worth it.
- *Alternative: let `emitKey(key)` repeat by itself until `emitKey(key, true)`.* Changes the
  behaviour of the existing entry point used by scripted walks, which send a press and a release
  back to back; kept separate.

### D2 — Send the evdev sequence, not the desktop keyboard's

Repeats are presses with `autoRepeat = true` and no releases in between; the final release is
plain. This is what the device sends, so the event takes the device branches of
`InputController::eventFilter()`: the stored owner for repeats, no restart of the POWER hold timer,
and an immediate release instead of the 150 ms deferral the desktop keyboard needs.

- *Alternative: X11-style release/press pairs.* Would exercise the 150 ms deferral instead of the
  device path and delay every release by 150 ms. Rejected.

### D3 — Every emulated key repeats

The device kernel repeats every key of the keypad input device, so the simulator does the same,
including HOME, BACK, VOICE and POWER. `ButtonNavigation` ignores repeats while a `long_press` is
pending, and `InputController` does not restart the POWER hold timer on a repeat, so long press and
the 3 s POWER hold keep working.

### D4 — `Button.qml` forwards press, release and cancel

`onPressed` → `pressSimulatorKey(key)`, `onReleased` and `onCanceled` → `releaseSimulatorKey(key)`.
The off-white tint stays bound to `mouseArea.pressed`.

## Risks / Trade-offs

Failure Mode Analysis (input ownership surface):

- [A hold never ends because the mouse release is lost (grab stolen, window hidden)] → `onCanceled`
  sends the release; `releaseSimulatorKey` also runs when the `Button` is destroyed while pressed
  (the window's `Loader` unloads its content when the window is hidden).
- [Repeats reach a new owner after a press opened a popup] → repeats carry the auto-repeat flag,
  so `InputController` routes them to the owner of the first press, as on the device.
- [`blockInput(true)` while a key is held] → the event filter swallows the repeats and the release,
  as it does for device keys; the timer stops on the release. Same as the device.
- [The 3 s POWER hold restarts on each repeat] → the hold timer only starts on a plain press
  (`inputController.cpp:228`); repeats are flagged.
- [Device behaviour changes] → the new invokables are only called from `Button.qml`, which is
  only loaded in `DEV`. The event filter is unchanged.

Trade-offs:

- The delay and rate are fixed at the device values. Developers who want a faster scroll still
  have the computer keyboard.
- Resource impact: one `QTimer` object in `InputController` (a few dozen bytes on every model);
  it runs only while a simulator button is held, which happens only in `DEV`. No CPU, battery,
  frame-rate or binary-size effect on the device; the first press is sent synchronously as today,
  so the input-to-command latency is unchanged. Negligible by inspection, no measurement needed.

## Migration Plan

- No data or configuration migration. Rollback is a revert of the commit.
- Verification target: the desktop simulator (`UC_MODEL=DEV`) against the Remote-Core Simulator,
  because the button simulator exists only there. Walk: hold DPAD_DOWN on a list (selection keeps
  moving), hold VOLUME_UP on an activity (repeated volume commands in the core simulator log), hold
  HOME (long press opens the page menu once), hold POWER 3 s (power off menu), short-click
  DPAD_DOWN (one step). `uc.ui.input` at debug level shows each press with its owner.
- The unit test covers the event sequence and timing; the device path needs no device run because
  it does not change.

## Open Questions

None. The Remote Two and the Remote 3 firmware set the same auto-repeat delay and period
(600 ms / 150 ms), so the simulated Remote Two keypad uses the device values.
