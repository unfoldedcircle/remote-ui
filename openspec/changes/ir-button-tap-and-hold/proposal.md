## Why

A physical button mapped to a simple command of an IR remote sends the plain command at key-down
and only starts the continuous IR repeat with the first auto-repeat, about 600 ms later. Every
hold reaches the device as a single press, a gap, then a held key. Devices that use a different
function for a held key, such as skip on a press and fast-forward or search while held, always run
the single-press function first, and the gap interrupts search functions
([ucd3-firmware#108](https://github.com/unfoldedcircle/ucd3-firmware/issues/108),
[#69](https://github.com/unfoldedcircle/remote-ui/issues/69)). With the current behaviour these
users can't reach the held function at all.

This is the quick fix until press-and-hold is available for all remotes (`docs/tasks/key-handling`,
rule 2). It is limited to IR remotes: Bluetooth remotes and the remotes of integration drivers have
no held state in the released core and Integration API, so the change would only delay their
first step.

## What Changes

- For an IR remote (a remote of the core's own integration `uc.main`), a physical button whose
  short press is a simple command (a `cmd_id` without `remote.`) and that has no long-press mapping:
  - the press sends nothing;
  - released before the first auto-repeat (a tap), the plain command is sent once, at the release;
  - held, the first auto-repeat starts the continuous IR repeat with `remote.send {command, repeat: 4}`,
    as every auto-repeat does today, and the release sends `remote.stop_send`. The plain command is
    not sent.
- This applies on the remote's screen and to the button mapping of an activity.
- Unchanged:
  - Bluetooth remotes and integration remotes;
  - buttons with a long-press mapping;
  - commands containing `remote.`;
  - the `press: true` flag, which only the remote's screen sends.
- The button handling of a remote entity (which command a press, an auto-repeat and a release send)
  moves from `Activity.qml` and `Remote.qml` into the C++ remote entity, as ADR 0012 requires for
  the code a change touches. The media player and other entity cases of the activity mapping stay
  in QML.
- The input-to-command latency rule measures an IR tap from the release, as it already does for a
  key with a long-press action.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `remote-entity`:
  - new requirement "Tap and hold of an IR remote's simple command";
  - "Physical button mapping on the remote's screen" refers to it, and its "Held simple command"
    scenario applies to integration and Bluetooth remotes.
- `activities`: "Physical button mapping in an activity" refers to the new rule, and the "Held
  button on an IR command" scenario sends nothing on the press.
- `platform-constraints`: "Input-to-command latency" names the release of an IR tap as the
  triggering input, with a new scenario.

## Impact

- **Hardware models:** Remote Two and Remote 3 alike; no hardware backend involved.
- **Core-API:** `execute_entity_command` with a simple command, `remote.send {command, repeat}` and
  `remote.stop_send`, all sent today. No new message or field.
- **Core dependency:**
  - For RC5, RC5X, RC6, 32-bit RCMM and toggling PRONTO codes, the core inverts the toggle bit
    only for a send without `repeat`. Today the plain command at key-down hides this; without it,
    no hold inverts the toggle bit.
  - **Release gate (confirmed by the maintainer):** this change and the matching remote-core
    fix (the toggle bit at the start of a continuous IR repeat) ship together, in the same
    firmware release, and neither ships without the other. The core fix on its own would turn a
    hold of today's UI into two presses on RC5/RC6 devices. UI and core ship together anyway
    (ADR 0005).
- **Dock:** no change. A `repeat` send already starts a continuous IR repeat without a separate
  single code.
- **Code:**
  - `src/ui/entity/remote.h` / `.cpp`: new entry points for a mapped button's press, auto-repeat
    and release.
  - `src/qml/components/entities/activity/deviceclass/Activity.qml` and
    `src/qml/components/entities/remote/deviceclass/Remote.qml` forward to them.
  - `src/ui/inputController.h`: loses its `repeatCount` property, which the remote entity now owns.
- **Tests:** a new `testRemoteEntity` target in `test/ui/CMakeLists.txt`.
- **Third-party code and assets:** none.
- **Docs:** `CHANGELOG.md`.
