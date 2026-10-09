## Why

The tile of a switch and its control screen always send `switch.toggle`, even when the switch
does not have the `toggle` feature. An integration that only offers `on_off` then gets a command it
never declared: it may reject it, or handle it only by accident. The Core-API defines what the
remote does instead: without the `toggle` feature it uses the current state of the switch and
sends `on` or `off`. The UI already does this for a light, but not for a switch.

## What Changes

- Toggling a switch from its tile (tap on the icon, or the quick action with DPAD_MIDDLE) or from
  its control screen (tap on the button, DPAD_MIDDLE, POWER) sends:
  - `switch.toggle` when the switch has the `toggle` feature, as today;
  - otherwise `switch.off` when the switch is On, and `switch.on` in any other state (Off, Unknown,
    Unavailable), the same rule the light control screen already follows.
- A switch with neither `on_off` nor `toggle` still has no quick action on its tile.
- Unchanged: commands taken from the configuration (button mappings, UI pages, activity and macro
  sequences) are sent exactly as configured, so a configured `switch.toggle` stays
  `switch.toggle`. The group switch keeps sending `switch.on` / `switch.off`. An Unavailable switch
  still refuses the command with "<name> is unavailable".

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `entity-detail-controls`: a new requirement "Switch toggle command" states the rule;
  "Tile quick action" and "Switch control screen" refer to it instead of naming `switch.toggle`,
  and the "Switch with only on_off" scenario sends `switch.off`.
- `entity-commands`: the "Switch toggled from its tile" scenario of "Entity command request"
  applies to a switch with the `toggle` feature.

## Impact

- Hardware models: Remote Two and Remote 3 alike; no hardware involved.
- Core-API: `execute_entity_command` with `switch.on`, `switch.off` and `switch.toggle`, all
  defined for the switch entity today; no core change and no dependency on a newer core. The
  fallback is the one the Core-API switch entity documentation prescribes for a switch without
  the `toggle` feature.
- Integrations: only switches that do not declare `toggle` are affected; they now receive the
  `on` and `off` commands they declared instead of `toggle`.
- Code: `Switch::toggle()` (`src/ui/entity/switch.cpp`); every switch control already goes
  through it, so no QML changes.
- Tests: a new unit test target for the switch entity (`test/ui/test_switch_entity.cpp`,
  `test/ui/CMakeLists.txt`).
- Third-party code and assets: none.
- Docs: `CHANGELOG.md`.
