## Context

See proposal.md, "Why". The full analysis of key handling is in `docs/tasks/key-handling/README.md`.

### Current State Analysis (commit `0783691`, `main`)

**Key path.**

- A held key arrives as one key-down, an auto-repeat key-down every 150 ms after a first delay of
  about 620 ms (firmware `repeat-delay=600:repeat-rate=150`), and one key-up.
- `InputController` delivers the release to the owner of the press. It defers a release flagged
  as auto-repeat by 150 ms (`src/ui/inputController.cpp:344-357`).
- `ButtonNavigation.onKeyPressedFor` (`src/qml/components/ButtonNavigation.qml:478-510`):
  - without a `long_press` handler: `pressed` on the first press, then `pressed_repeat` on every
    auto-repeat;
  - with a `long_press` handler: an 800 ms timer; `pressed` runs at the release when the timer is
    still running, and auto-repeats are ignored.
- `onKeyReleasedFor` (`:512-531`) then always runs `released`. Nothing here changes.

**Remote entity button mapping, built in QML twice.**

- Remote screen, `src/qml/components/entities/remote/deviceclass/Remote.qml:69-130`:
  - `canRepeat = !cmd_id.includes("remote.")` (`:79`);
  - `pressed` sends `cmd_id` with `params` (`:81-86`);
  - `pressed_repeat` sends `remote.send {command, repeat: ui.inputController.repeatCount, press: true}`
    for a simple command, otherwise `cmd_id` verbatim (`:88-104`);
  - `released` sends `remote.stop_send` for a simple command (`:107-113`);
  - `long_press` sends its `cmd_id` (`:118-123`).
- Activity screen, `src/qml/components/entities/activity/deviceclass/Activity.qml:255-369`:
  - the same rule inside a `switch (e.type)`, where `e` is the short-press entity resolved once
    per mapping update (`:272`);
  - `case EntityTypes.Remote` in `pressed` (`:290-296`) and `pressed_repeat` (`:315-331`);
  - `pressed_repeat` sends `remote.send {command, repeat}` **without** `press` (`:319-323`);
  - `released` sends `remote.stop_send` for a remote and a simple command (`:340-350`).
- Both send through `EntityController.onEntityCommand` directly. ADR 0012 names this as a
  deviation to move to C++ "by the change that next touches the screen".
- `ui.inputController.repeatCount` (`src/ui/inputController.h:24, 68, 164`, constant 4) is read
  only by these two call sites.

**Remote entity in C++.**

- `src/ui/entity/remote.h` / `.cpp` has `turnOn` / `turnOff` / `toggle` and the button mapping
  options, and no button handling.
- `Base::command(entityId, command, params)` is connected to `EntityController::onEntityCommand`
  (`src/ui/entity/entityController.cpp:561`), the same entry point the QML calls.
- `Base::getIntegration()` (`src/ui/entity/entity.h:150`) holds `integration_id` from the
  Core-API (`src/core/core.cpp:2141`).
- No unit test covers the remote entity.

**Commands.**

- A `remote.send` with `repeat` is a repeat command: tracked separately, never deduplicated, never
  resent after a wakeup (`src/ui/entity/commandRetryPolicy.cpp:9-11`,
  `src/ui/entity/entityController.cpp:1064-1070`).
- Any other identical command is dropped while it is pending.
- Known gap (`entity-commands`): this path doesn't apply the unavailable-entity refusal.

**Core (remote-core, current `main`).**

- `uc.main` is the core's own integration for IR remotes; BT remotes are `uc_bt.main`.
- A simple command is repacked to `remote.send {command}` (`crates/core/src/intg/messages.rs:451-476`).
- For an IR remote, `remote.send {command, repeat}` becomes a dock `ir_send` with `repeat`. The dock
  starts a continuous IR repeat and prolongs it on every further request for the same code;
  `remote.stop_send` stops it.
- The `press` flag is ignored for IR remotes. BT remotes ignore `repeat` and `stop_send`;
  integration remotes get the old repeat mode.
- Toggle bit: `get_new_toggle_state()` (`crates/core/src/actors/ir/emitter/actor.rs:149`) inverts
  the toggle only for a send without `repeat`. The fix is tracked as a remote-core issue; see
  Risks.

### Constraints

- ADR 0012: logic in C++, QML forwards input.
- ADR 0009: unit tests for new logic.
- ADR 0005: the UI sends the user's intent; no new Core-API.
- ADR 0007: the screens keep their `ButtonNavigation` idiom.
- ADR 0008: both models alike.

## Goals / Non-Goals

**Goals:**

- An IR remote's simple command reaches the device as a tap or as one held key, never as a tap
  followed by a hold.
- The remote entity's button rule exists once, in C++, with unit tests, and both screens use it.
- Unchanged for everything else: BT and integration remotes, buttons with a long press, commands
  containing `remote.`, the `press` flag per screen, all other entity types in the activity
  mapping.

**Non-Goals:**

- Press-and-hold from key-down for all remotes, and pacing repeats to one in flight
  (`docs/tasks/key-handling`, rule 2, the problems listed as 2 and 3).
- `remote.stop_send` after a long press (problem 6) and lost releases (problem 7): unchanged.
- Moving the media player, macro and other cases of the activity mapping to C++; they stay as
  ADR 0012 deviations.
- The unavailable-refusal gap of the QML mapping path: unchanged. The C++ entry points send
  through the same `onEntityCommand`.

## Decisions

### D1 — The remote entity owns its mapped-button handling

`Remote` gets three `Q_INVOKABLE` entry points, called with the button name and the short-press
`cmd_id` and `params`:

- `mappedButtonPressed(button, cmdId, params, hasLongPress)`;
- `mappedButtonRepeated(button, cmdId, params, sendPress)`;
- `mappedButtonReleased(button, cmdId, params, hasLongPress)`.

Each emits `Base::command` with the command to send, or nothing. `Remote.qml` and the
`EntityTypes.Remote` cases of `Activity.qml` only forward to them; `canRepeat` leaves both files.

- *Alternative: QML closures with a captured `held` flag (the first sketch).* About 10 lines per
  file, but it adds decision logic in QML, which ADR 0012 rejects, and it can't be unit tested.
  Rejected.
- *Alternative: move the whole activity button mapping to C++.* It would correct the ADR 0012
  deviation completely, but it's a large refactor of every entity case and no longer a quick
  fix. Rejected for this change; it stays a known deviation.
- *Alternative: a free policy function, like `commandRetryPolicy`.* The rule needs per-button
  state between press and release, which belongs to an object; the remote entity is the one both
  screens already hold. Rejected.

### D2 — The rule

`simple = !cmdId.contains("remote.")` (unchanged), `ir = getIntegration() == "uc.main"`, and
`tapRule = ir && simple && !hasLongPress`.

| Entry point | `tapRule` | otherwise (today's behaviour) |
|---|---|---|
| pressed | forget that `button` is held; send nothing | send `cmdId` with `params` |
| repeated | — (see below) | — |
| released | `button` held: send `remote.stop_send`, forget it; not held: send `cmdId` with `params` | simple: send `remote.stop_send` |

The repeated entry point is the same for every remote, so it isn't split by `tapRule`. For a
simple command it marks `button` as held and sends `remote.send {command: cmdId, repeat: 4}`, plus
`press: true` when `sendPress`. Otherwise it sends `cmdId` with `params`. `ButtonNavigation` never
calls it for a button with a long press.

- The held state is a `QSet<QString>` of button names.
- It is cleared at every press, so a release that never arrived affects only that one hold. The
  next press of the button is again a tap or a hold.
- Unlike a QML closure, the state survives a live `button_mapping` update in the middle of a hold.
- Integration id `uc.main` is the core's own classification of an IR remote
  (`is_internal_ir_remote_entity`), and it can't match BT (`uc_bt.main`) or driver remotes.
- *Alternative: send the plain command at the first auto-repeat.* It is a single code again, the
  one thing the change removes. Rejected.
- *Alternative: send `remote.send {repeat}` already at key-down (rule 2).* A tap would become a
  short hold, and whether the device sees a tap or a hold would depend on how fast the
  `remote.stop_send` reaches the dock. Rejected for the quick fix.

### D3 — The remote entity owns the repeat count

The `repeat` value 4 is part of the `remote.send` command the entity builds, so it becomes a
constant of `Remote`. `InputController::repeatCount` (`inputController.h:24, 68, 164`) loses its
only readers and is removed.

- *Alternative: pass it from QML as a parameter.* That's a value the caller can't choose, and
  `InputController` would keep owning a Core-API detail. Rejected.

### D4 — Each screen keeps its `press` flag

The remote screen passes `sendPress = true` and the activity passes `false`, as today. Making them
equal changes what integration remotes with the `stop_send` feature receive (the core's
press-and-hold mode versus the old repeat mode). That is the driver regression this change avoids;
it belongs to rule 2.

## Risks / Trade-offs

This touches input handling (a risky surface), so the failure modes come first:

- [Toggle bit, RC5 / RC5X / RC6 / RCMM / PRONTO `A|B`: a hold no longer starts with a plain send,
  so the core never inverts the toggle bit for a hold. "Tap A, hold A" and "hold A, hold A" reach
  the device with the same toggle, and a strict receiver takes the new hold as the old press still
  held] → the remote-core fix (invert the toggle at the start of a new continuous IR repeat) is a
  release prerequisite. This change must not ship in a firmware without it. No UI-only
  workaround exists, because only a plain send inverts the toggle.
- [Release lost (input blocked, screen destroyed during the press)] → today the command was sent
  and only `remote.stop_send` was lost; now a tap sends nothing. The next press starts clean
  (D2). This is rare, because the release goes to the owner of the press.
- [Screen closed during a hold] → as today, `remote.stop_send` may be lost. The dock's repeat
  runs out after about 4 repeats.
- [Same remote entity used by the activity and the remote screen] → only one screen owns a key at
  a time. The state is keyed by button, and each press resets it.
- [`e` not resolved in the activity when the mapping was built] → as today, nothing is sent.
- [A tap is sent later than today: at the release instead of the press, typically 80–150 ms] →
  that's the intent, the same as for a key with a long press. The latency rule now measures it
  from the release (`platform-constraints` delta). The request goes out synchronously in the
  release handler, within the 20 ms budget.
- [A held IR key starts about 620 ms after key-down, with one plain step fewer than today] → the
  intended behaviour: a held key, as a physical IR remote sends it. Volume and d-pad holds on IR
  remotes feel this change. It goes into the CHANGELOG entry.
- [Devices that relied on the plain code at key-down before the repeat (a "first step at once")]
  → accepted. A tap still sends the plain code, and a hold now behaves as on the device's own
  remote.

**Resource impact:** no timer, no polling and no new Qt module; one `QSet` insert and lookup per
key event. The number of requests per hold drops by one. Negligible.

## Migration Plan

- No data or configuration migration. Rollback is reverting the commit.
- **Release order:** only in a firmware release that contains the remote-core toggle-bit fix.
- **Verification:**
  - **Unit tests** (`testRemoteEntity`) for the rule in D2 and the unchanged paths.
  - **`DEV` simulator against the core-simulator**, for the command sequence per button (debug
    log `lcEntityController`): an IR remote tap and hold, the same on an integration remote, a
    button with a long press, and a `remote.` command. The simulator is enough for the messages.
  - **Device with a dock** (Remote Two and Remote 3), for the IR output, which only a device
    shows:
    - an NEC device with a different function on hold (tap = skip, hold = search);
    - an RC5 or RC6 device (needs the core fix): tap A then hold A, hold A then hold A;
    - an integration and a BT remote, unchanged.
  - The keys are walked with the keypad, not by touch.

## Open Questions

None.
