## Context

### Current State Analysis (commit `808c9d5b`, `main`)

- `Switch::toggle()` (`src/ui/entity/switch.cpp:60-62`) sends `switch.toggle` unconditionally.
- Every control of a switch that toggles goes through it:
  - the tile quick action (`src/qml/components/entities/Base.qml:288-299`), which is only set
    when the switch has `on_off` or `toggle`;
  - the switch screen (`switch/deviceclass/Switch.qml`: DPAD_MIDDLE `:18`, POWER `:23`, the
    button `:91`);
  - the outlet screen (`switch/deviceclass/Outlet.qml`: `:18`, `:23`, `:118`).
- The group switch (`src/qml/components/group/Base.qml:113`) calls `turnOn()` / `turnOff()`
  and never sends a toggle.
- `Light::toggle()` (`src/ui/entity/light.cpp:66-76`) already applies the fallback: `light.toggle`
  with the `toggle` feature, otherwise `light.off` when On and `light.on` in any other state.
  `Remote::toggle()` (`src/ui/entity/remote.cpp:57-63`) always sends `remote.on` / `remote.off` by
  state.
- The Core-API switch entity documentation, `toggle` feature: "If there's no native support, the
  remote will use the current state of the switch to send the corresponding on or off command."
  The core forwards `switch.toggle` to the integration unchanged; it does not apply the fallback
  either.
- An Unavailable switch: `EntityController::refuseUnavailableEntity()`
  (`src/ui/entity/entityController.cpp:915`) refuses the emitted command, whichever it is, with
  "<name> is unavailable".
- Duplicate suppression keys a pending command by entity, `cmd_id` and params
  (`entity-commands`); it does not depend on which command a toggle picks.
- Tests: no unit test covers the switch entity.

### Constraints

- ADR 0005: the core owns the state; the UI picks the command from the last state the core
  reported.
- ADR 0009: a bug fix comes with a unit test that fails without it.
- ADR 0012: the choice is logic and belongs in C++, not QML.
- ADR 0008: the Remote Two and the Remote 3 behave the same.

## Goals / Non-Goals

**Goals:**

- The UI's own switch controls never send `switch.toggle` to a switch without the `toggle`
  feature; they send `switch.on` or `switch.off` from its state instead.
- Switches and lights follow the same rule.

**Non-Goals:**

- Commands taken from the configuration (button mappings, UI pages, activity and macro sequences):
  the `entity-commands` capability requires them to be sent exactly as configured.
- `Remote::toggle()`, which ignores the `toggle` feature of a remote entity.
- The `readable` option of a switch, which the UI parses but does not use.

## Decisions

### D1 — The fallback lives in `Switch::toggle()`, as in `Light::toggle()`

`Switch::toggle()` sends `switch.toggle` when the entity has the `toggle` feature, otherwise
`switch.off` when the state is On and `switch.on` in any other state. All tile and screen controls
call it, so none of them changes.

- *Alternative: decide in QML at each call site.* Seven call sites in three files, and logic in
  QML (ADR 0012). Rejected.
- *Alternative: let the core translate the command.* That would also cover configured commands and
  other clients, but it is a remote-core decision, and the Core-API assigns the fallback to the
  remote, which the UI already applies to lights. Not needed for this fix; recorded as an open
  question.

### D2 — A state other than On sends `switch.on`

Off, Unknown and Unavailable send `switch.on`, as for a light. An Unavailable switch refuses the
command anyway.

- *Alternative: send `switch.toggle` while the state is unknown.* That is the undeclared command
  this change removes. Rejected.
- *Alternative: send nothing while the state is unknown.* A switch that has not reported a state
  yet could then never be switched on from its tile or screen. Rejected.

## Risks / Trade-offs

- [The last reported state is stale, e.g. the integration missed an event] → the remote sends
  `switch.on` to a switch that is already on, the device does not change and the tile stays Off;
  a native toggle would have flipped it. The same holds for lights today, and a switch without the
  `toggle` feature has no native toggle to rely on.
- [A switch without `toggle` that never reports a state (`readable` = false)] → every press sends
  `switch.on`, so the tile and the screen cannot switch it off. Before, `switch.toggle` reached an
  integration that did not declare it. The Core-API asks integrations to avoid unreadable
  switches; how the UI handles `readable` is a separate decision.
- [Two quick taps while the first `switch.on` is pending] → the second tap picks the same command
  from the unchanged state and is suppressed as a duplicate, as `switch.toggle` was before.

Resource impact: none. The choice is one comparison on state the entity already holds; no timers,
no memory, no new modules.

## Migration Plan

- No data or configuration migration; rollback is a revert.
- Verification target: unit tests of `Switch::toggle()` for every feature and state combination,
  which fail without the fix. The controls, the command path and the core are unchanged, so the
  only observable difference is the `cmd_id` the entity emits. No device run is needed: no hardware
  path is involved, and both models run the same code.

## Open Questions

- The core forwards a configured `switch.toggle` (button mapping, sequence) to a switch without
  the `toggle` feature unchanged, and so does it for other API clients. Whether the core should
  apply the Core-API fallback there is a remote-core question, outside this change.
