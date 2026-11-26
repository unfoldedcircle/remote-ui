## Context

See proposal.md, "Why". The key and IR hold behaviour this builds on is `ir-button-tap-and-hold`
(#93).

### Current State Analysis (commit `88f573d`, `feat/ir-button-tap-and-hold`)

**The page button.**

- `ButtonIcon.qml` is the only page button, used by exactly two screens: `Remote.qml:34` and
  `Activity.qml:131`.
- Its `HapticMouseArea` (`ButtonIcon.qml:77-83`) has only `onClicked`, which calls the `trigger`
  function the screen passed in. `clicked` fires at the release, only when the finger is lifted
  inside the button and the press wasn't cancelled. So a tap sends at the release, a long touch
  sends the same single command at the release, and a press that turns into a swipe sends nothing.
- The haptic click comes at the press (`HapticMouseArea.qml`).

**The screens.**

- **Remote screen:** `parsePageItems()` (`Remote.qml:21`) creates a `ButtonIcon` per text or icon
  item. Its `trigger` (`Remote.qml:42-58`) sends `remote.send {command}` for a simple command,
  otherwise `cmd_id` with `params`.
- **Activity screen:** `parsePageItems()` (`Activity.qml:120`) creates the same button. Its
  `trigger` (`Activity.qml:139-143`) calls `triggerCommand()` (`:102`) for any entity type:
  remote, media player, light, macro, activity.
- **Pages:** both screens put their pages in a `PathView` (`Remote.qml:178`, `Activity.qml:584`).
  A drag past the threshold steals the press, and the button's `MouseArea` gets `canceled`.
  - Page delegates create their buttons in `Component.onCompleted` (`Remote.qml:211-213`,
    `Activity.qml:761`).
  - A `user_interface` update re-creates the pages, so a held button can be destroyed.

**The remote entity (from #93).**

- `mappedButtonPressed` / `Repeated` / `Released` (`remote.cpp:65-114`) hold the rule for a
  physical button, with the held buttons in `m_heldButtons` (`remote.h:151`) and
  `kHoldRepeatCount = 4` (`remote.h:128`).
- `mappedButtonRepeated` sends `remote.send {command, repeat 4[, press]}` for a simple command and
  marks the button held, otherwise it resends the command verbatim. `mappedButtonReleased` sends
  `remote.stop_send` for a held simple command.

**Elsewhere.**

- Precedent: the desktop button simulator turns a held mouse press into key repeats with a C++
  timer (600 ms, then every 150 ms; `inputController.cpp:14-15, 68-73, 108-128`). Its button
  releases on `canceled` and in `Component.onDestruction` (`button-simulator/Button.qml:28-37`).
- The Core-API page item (`UserInterfaceItem`) has no repeat or long-press field.
- The `remote-entity` spec says "holding a page button does not repeat"
  (`openspec/specs/remote-entity/spec.md:57`).

### Constraints

- ADR 0012: the decision what to send and when lives in C++; QML forwards the touch.
- ADR 0009: unit tests for new logic.
- ADR 0016: the haptic click goes through the `Haptic` backend from QML, as every other click.
- ADR 0008: both models alike.

## Goals / Non-Goals

**Goals:**

- A long touch on a page button of a remote sends what a held physical button sends.
- No change for taps, swipes, excluded commands and non-remote items.
- The timing is defined in one place.

**Non-Goals:**

- Holding page buttons of other entity types (media player volume etc.): a later extension.
- A setting for the touch timing: only the single constant now.
- A per-button repeat flag in the Core-API.

## Decisions

### D1 — The remote entity owns the touch hold

`Remote` gets three `Q_INVOKABLE` touch entry points, keyed by a touch id:

- `touchPressed(touchId, cmdId, params, sendPress)`:
  - if the command repeats (D4), remember the touch and start the hold timer;
  - otherwise do nothing;
  - never send.
- `touchReleased(touchId) → bool`:
  - stop the timer;
  - if the hold had started, end it like a physical release (`mappedButtonReleased`) and return
    `true`; also `true` when this touch's hold already ended without its release (D7);
  - otherwise return `false`, and the button sends its tap as today.
- `touchCanceled(touchId)`:
  - stop the timer;
  - end a started hold like a release;
  - never send a tap.

`touchPressed` resolves the simple command the hold repeats (D4). When the timer fires the first
time, it calls `mappedButtonRepeated(touchId, command, {}, sendPress)` and then emits
`touchHoldStarted(touchId)`, so a release in a handler of the hold start ends a hold whose first
repeat is out. It repeats every period after that. The hold's end calls
`mappedButtonReleased(touchId, command, {}, false)`.

- The repeats and the stop are therefore exactly those of a held physical button: for every remote
  kind, the IR rule from #93, and `press` per screen.
- The tap stays in QML and is untouched. The return value of `touchReleased` only suppresses it
  after a hold, so every tap message stays byte-for-byte what it is today.
- One touch hold per entity. A press with another touch id first cancels the previous one. Qt's
  `MouseArea` only follows the first touch point, so this is only a safeguard.
- The touch id is `touch:<screen entity id>:<page>:<item>`. It can't collide with a physical
  button name in `m_heldButtons`. The screen's entity id keeps the remote's own screen and an
  activity apart, which can both show a button of the same remote at the same position.
- *Alternative: a QML `Timer` in `ButtonIcon`.* The timing and the decision which commands repeat
  would be QML logic (ADR 0012), and it couldn't be unit tested. Rejected.
- *Alternative: let the tap go through `mappedButtonReleased` too.* On the remote screen a simple
  command's tap is `remote.send {command}`, from a physical button it is the plain `cmd_id`. The
  core treats both the same, but the message would change. Rejected for "no change for taps".

### D2 — A touch never sends at touch-down

A physical key of an integration or Bluetooth remote sends at key-down. A touch can't: it may still
become a swipe, and today a swipe that starts on a button sends nothing. On every remote kind, a
touch is therefore a tap at the release or a hold from 600 ms on. The only difference to a
physical key is on integration and Bluetooth remotes, whose held key also sends the plain command
at key-down.

### D3 — One place for the timing

`remote.h` defines `kTouchHoldDelayMs = 600` and `kTouchRepeatPeriodMs = 150` next to
`kHoldRepeatCount`. They match the remote's key repeat (firmware `repeat-delay=600:repeat-rate=150`),
so a long touch feels like a long key press. Both are read in one place, so a later setting only
replaces where the values come from.

- *Alternative: share the constants with the button simulator's key repeat.* The simulator mirrors
  the firmware's key repeat, while the touch timing may become a setting of its own. Kept separate.

### D4 — Which commands repeat on a touch, and how

`touchHoldCommand(cmdId, params)` returns the simple command a held button repeats, or nothing:

- **A simple command** repeats as itself.
- **A `remote.send` or `remote.send_cmd` whose only param is `command`** repeats as that simple
  command: `remote.send {command, repeat: 4}` and `remote.stop_send`, a real hold.
  - Resent verbatim, the same command would go out as a series of single sends. For an IR remote
    that's taps, not a hold, and with a toggling protocol every one would invert the toggle bit.
  - Verbatim commands are also ordinary commands for `EntityController`: deduplicated while one is
    in flight, resent after a wakeup, and offered for "Try again" on a failure.
- **Every other command only taps:** sequences, `remote.on` / `off` / `toggle`, and commands with
  params of their own (`repeat`, `delay`, `hold`), whose meaning a repeat would change.
- **Power commands only tap:**
  - a resolved name of `on`, `off` or `toggle`, or any name containing "power", in any letter case;
  - command names of integration remotes are free text (`KEY_POWER`, `PowerOn`), so a name match is
    the only rule that covers them;
  - a false positive (`POWER_SAVING`) only means "no repeat"; a power command without "power" in
    its name (`STANDBY`, a localised name) still repeats.
- The rule is a static function of the remote entity, unit tested.

### D5 — The haptic click at the hold start

`ButtonIcon` listens to `touchHoldStarted` of its entity and, while it is touched itself, plays
`Haptic.Click` for its own touch id. The feedback stays in QML with the other clicks (ADR 0016);
C++ only says when.

### D6 — `ButtonIcon` forwards press, release, cancel and destruction

The screen passes the remote entity, the touch id, the command and `sendPress` to the button; a
button without an entity behaves as today.

- `onPressed` → `touchPressed`, and the button marks its touch active.
- `onReleased` → `touchReleased`. `onReleased` fires before `onClicked`, so the button remembers
  whether a hold ended, and `onClicked` only sends the tap when none did. A release outside the
  button ends a hold and sends no tap, as today.
- `onCanceled` → `touchCanceled`. `Component.onDestruction` → `touchCanceled` only while its own
  touch is active, so a destroyed button never ends another button's touch.
- **When the hold starts,** the button sets `preventStealing`, reset at the release and the cancel.
  `PathView` steals a press once the finger moves past its drag threshold (about 10 px), with no
  time limit, so a slight drift of a resting finger would end the hold in the middle. Set during
  a press, `preventStealing` takes effect at once (`QQuickMouseArea::setPreventStealing` sets
  `keepMouseGrab`, which `PathView` respects). Before the hold starts, a swipe still wins.

### D7 — A touch hold always ends

The hold ends only through the button, so every path where the button gets neither a release nor
a cancel needs its own end:

- **Touch input blocked.** In `LOW_POWER`, `InputController` discards touch and mouse events, the
  releases included (`inputController.cpp:238-250, 366-375`), and on eglfs hiding the window
  cancels no grab. `blockInput(true)` (a blocking wait) does the same for all input.
  `InputController` emits `touchInputBlocked()` in both cases, and
  `EntityController::cancelTouchHolds()` calls `cancelTouch()` on every remote. A hold ended this
  way remembers its touch id, so the release that arrives later returns `true` and sends no tap.
- **A failsafe limit:** `kTouchHoldMaxMs = 30000`. A hold that is still running after 30 s ends like
  a cancel. A physical button has no limit, but a touch hold is the UI's own timer, and a runaway
  repeat pins an amplifier's volume. The value is one constant next to the timing (D3).
- *Alternative: let `InputController` synthesize a touch cancel to the grabber.* It would cover
  every touch item, but it changes the input path of every screen. Rejected for this change.

The activity passes an entity only when the item's entity is a remote; other items stay as they
are.

## Risks / Trade-offs

This touches touch input on two screens (a risky surface), so the failure modes come first:

- [A swipe starts on a button] → before the hold, the `PathView` steals the press and nothing is
  sent, as today. After the hold started, the button keeps the press (D6), and the page swipes
  only after the finger is lifted and swiped again.
- [The release is never delivered] → touch input blocked: D7, ended at once. Any other lost
  release: the 30 s limit (D7).
- [The button is destroyed during a hold (screen closed, pages rebuilt, delegate recycled)] →
  `Component.onDestruction` cancels, so `remote.stop_send` goes out. Without it, the dock repeats
  ~0.5 s more and stops by itself.
- [The entity is destroyed during a hold] → its timer goes with it; the dock stops by itself
  after ~0.5 s. Entities live as long as their configuration, so this needs a reconfiguration
  during a hold.
- [Slow or resting fingers turn into holds] → that's the requested behaviour. Power commands
  and sequences are excluded (D4), the haptic click tells the user a hold started, and the delay
  can be changed in one place (D3).
- [A power command under a name without "power" (e.g. `STANDBY`)] → it repeats on a hold, like
  the same command on a held physical button.
- [A slow press on an integration remote] → a touch of 0.6 s or more is a hold, and its first
  message is `remote.send {command, repeat: 4}`. A driver without press-and-hold support gets the
  old repeat mode and may run the command several times, for example MUTE or PLAY_PAUSE.
  Physical keys of that remote behave the same. → Product decision, see Open Questions.
- [An IR hold released right after the haptic click] → `remote.send {repeat}` and
  `remote.stop_send` follow each other closely. The dock transmits the frame in flight, so at
  least one frame goes out. To check on a device.
- [An activity item whose remote isn't loaded when the page is built] → the button stays tap-only
  until the pages are rebuilt. Activity screens open after their entities are loaded.
- [Touch and a physical key of the same remote at the same time] → separate ids in
  `m_heldButtons`; the touch timer belongs to the touch.
- [The toggle bit] → a touch hold uses the same messages as a key hold, covered by the
  remote-core fix that ships with #93.
- [Unavailable entity] → page buttons don't apply the unavailable-entity refusal (known gap),
  unchanged.

**Resource impact:** one `QTimer` per remote entity, running only while a finger holds a page
button; no polling, no new Qt module. Negligible.

## Migration Plan

- No migration. Rollback is reverting the commit.
- **Release order:** with or after #93 (stacked on it), in the release that has the remote-core
  toggle-bit fix.
- **Verification:**
  - **Unit tests** in `testRemoteEntity`:
    - tap, hold, cancel before and after the delay, a release right at the hold start, a second
      touch id, the period, the 30 s limit (shortened), `cancelTouch()` and the tap suppression
      after it, destruction of the entity during a hold;
    - the D4 rule;
    - the timing, with the constants;
  - `testEntityController`: `cancelTouchHolds()` ends a remote's hold;
  - `testInputController`: `touchInputBlocked()` at `blockInput(true)` and at low power, not in
    `DEV`.
  - **`DEV` simulator against the core-simulator, with the mouse:** a touch can be tested there,
    unlike physical keys. Short and long touches on an IR remote and on an integration remote, a
    swipe that starts on a button before and after the hold started, a power command, and an
    activity item of a remote and of a media player. Check the command sequence in the
    `lcEntityController` debug log; the haptic click needs a device.
  - **Device, Remote Two and Remote 3:** the swipe interplay and the feel of the timing; the Dock
    log for the messages.

## Review

An independent adversarial review (Claude Fable, high effort) of the first implementation found,
now addressed:

- **critical:** a hold never ended when the release was discarded in low power or during a
  blocking wait → D7;
- **major:** a slight drift of the finger let the `PathView` steal the press and end the hold →
  `preventStealing` once the hold starts (D6);
- verbatim repeats of `remote.*` commands were ordinary commands (deduplicated, resent after a
  wakeup, "Try again") and, for an IR remote, taps instead of a hold → D4: a single command is
  held like a simple command, every other command only taps;
- the touch id wasn't unique across screens, and a destroyed button could end another button's
  touch → D1, D6;
- `on` / `off` / `toggle` inside `remote.send` repeated → D4;
- test gaps: the period, the limit, a release at the hold start, entity destruction, a flaky wait.

## Open Questions

- **Integration remotes and slow presses** (Risks): keep touch holds for integration remotes as
  for their physical keys, or limit touch holds to IR and Bluetooth remotes? It changes nothing in
  the code structure, only which remotes `touchHoldCommand` allows.
