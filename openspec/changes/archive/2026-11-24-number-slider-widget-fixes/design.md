## Context

See proposal.md - Why. Current state at `6d8e89a`, the thirteen findings of #67 as verified:

1. **Keys step past a limit.** `Number.qml:37-57` calls `increment()` / `decrement()` for DPAD_UP /
   DOWN and VOLUME_UP / DOWN unconditionally; the stepper buttons are disabled at the limit
   (`Number.qml:176-197`).
2. **A failed `set_value` stays shown.** `Number::setValue` (`number.cpp:96-115`) shows the value as
   pending; only a reported `value` or the 3 s timer ends that.
   `EntityController::handleCommandFailure` (`entityController.cpp:1092`) notifies the user, not the
   entity.
3. **A write-only number steps for dropped commands.** `Number::stepBy` (`number.cpp:125-139`)
   changes the shown value when the command is issued. `EntityController::onEntityCommand`
   (`entityController.cpp:1024-1030`) drops a command while an identical one is pending.
4. **The slider counts from `max`.** `Number.qml:113-116`: `from: max`, `to: min`,
   `stepSize: step`. Qt's `Slider` snaps to multiples of `stepSize` from `from`.
5. **Number widget buttons at a limit.** `NumberWidget.qml:172-177`: always enabled for a number.
6. **Slider widget `buttons` at a limit.** `SliderWidget.qml:384-391`: always sends.
7. **Feature not in `features`.** `SliderWidget.qml:187-225` checks the type and the feature name.
8. **`color: entity` without a colour.** `SliderWidget.qml:92-93` takes the `color` feature for a
   known colour; `Light` builds its `QColor` only when `hue` or `saturation` change from their
   initial 0 (`light.cpp:156-175`), so it is invalid until then, and stays invalid for a light that
   reports hue 0 and saturation 0.
9. **Bar outside its range.** The drag starts at the unclamped entity value
   (`SliderWidget.qml:764`); `SliderStyle::dragValue` adds the distance, then clamps.
10. **Orientation from pixels.** `SliderWidget.qml:74`, `NumberWidget.qml:45`: `height > width`.
    `Activity.qml:119-122` sizes a cell from the page's grid.
11. **Unavailable look.** Slider and number widgets dim with `ui.unavailableOpacity` (0.15) and
    `ui.unavailableLabelOpacity` (0.5) and use `ui.unavailableMarkSize` (30). `SensorWidget.qml` and
    `SelectWidget.qml` dim nothing and draw the mark at 40.
12. **`ready_check: null`.** `activity.cpp:67-69` and `:287-290` use `QVariant::toBool()`.
13. **Option updates.** Only number, activity, remote and voice assistant override
    `Base::updateOptions`; the sensor applies its decimals only. Switch, light, climate and media
    player read their options in the constructor. Button, cover, select and macro read no options,
    so they have nothing to update.

## Goals / Non-Goals

**Goals:** the thirteen behaviours of proposal.md, each with a unit test where the logic is in C++.

**Non-Goals:**

- The other slider features keep their optimistic value for 3 s after a failed command; #67 names
  the number only, and they have no pending state in C++ yet.
- No limit check for the `buttons` style of the other features: a press at the end of the range
  already sends nothing; only the dimming is missing, and #67 does not ask for it.
- `tilt_position` stays unsupported in the slider widget.
- A write-only number still shows a `set_value` at once, also when the command fails.

## Decisions

- **The limit check is in `Number`, not in the callers.** `Number::stepBy` returns without sending
  at a limit, and `atMin` / `atMax` are properties. The keys, the screen's stepper, the number
  widget and the slider widget's buttons then share one condition (limit set, value known). A
  command from an activity's button mapping does not pass through the entity and is not checked,
  as before. Alternative: a guard in each QML caller; rejected, four copies of one rule (ADR 0012).
- **The controller tells the entity how its command ended.** `Base::commandFinished(command,
  params, success)` is called once per command: on the result, or on the failure that is not
  retried. A dropped duplicate and a refused command are never sent and never reported.
  `Number` ends its wait on a failed `set_value` whose value is the one it waits for, and steps a
  write-only number on an executed `increment` / `decrement`.
- **A write-only number steps when the command was executed, not when it was sent.** #67 asks for
  "only when the command was actually sent". The result is the stronger form: it also leaves the
  value alone when the core or the driver refuses the command, which the core-api note ("one step
  per command the driver receives") implies. The cost is that the value moves when the core
  answers, typically within the same frame; while the answer is outstanding the controller drops
  further identical steps anyway.
- **`setValue` starts the wait before it sends.** Without a connection the command fails inside
  `sendCommand`; the failure has to find the wait it ends.
- **The number screen's slider snaps with `SliderStyle::snap`.** The Qt `Slider` keeps its range
  and loses `stepSize`; the value shown and sent is `snap(value, min, max, step)`, the function the
  slider widget uses. The fill follows the finger between positions, as a discrete slider widget
  does. Alternative: `from: min` without the 180° rotation; rejected, Qt still cannot make `max`
  a position of a range that is not a whole number of steps.
- **A feature is checked against `features` once they are known.** Configured entities are bulk
  loaded without features. Like the touch slider, the slider widget fetches the entity once and
  judges it after `featuresChanged`; until then it is shown as supported, as today.
- **`Light::colorKnown`** is true once `hue` and `saturation` were both reported with a value. The
  colour is built then, also for hue 0 and saturation 0.
- **A bar drag from outside the range.** `SliderStyle::dragValue` clamps the start value first. The
  widget ignores the drag until it leaves the nearest end, so a drag towards that end neither
  changes the value shown nor sends the end of the range.
- **The orientation is a property the page sets** from the item's `size`; the widgets keep
  `height > width` as the default for other users. The number widget passes it to its slider.
- **The sensor and select widgets take the unavailable look of the slider and number widgets.**
  The design system defines that look and its tokens ("No value and unavailable"); the two older
  widgets predate it. Alternative: remove the dimming from the newer widgets; rejected, it would
  change the documented look.
- **`ready_check` is true unless it is the boolean `false`.**
- **Options are applied as a whole.** Each type gets `applyOptions()`, called by the constructor
  and by `updateOptions()`, as `Number` has. The core delivers the complete options, so a missing
  option is its default. Properties that were `CONSTANT` get a notify signal.

## Risks / Trade-offs

Failure modes of the touched Core-API command and event handling:

- [A command result arrives for a deleted entity] → `commandFinished` is called on the entity
  looked up by id at that moment; a missing entity is skipped.
- [`commandFinished` runs inside `sendCommand`] → only for a command that cannot be sent;
  `Number::setValue` has set its state before it sends, and `stepBy` holds none.
- [A retried command is reported twice] → the wakeup retry returns before the report; only the
  last failure or the result reports.
- [The feature fetch never answers] → the slider stays shown as supported, as today.
- [An option update resets a value the user is editing] → options are replaced, attributes are
  not; a slider being dragged keeps its drag (`syncFromEntity` returns while dragging).
- [Options delivered partially by a core that sends a delta] → not the case for the released
  core; the number entity already relies on complete options.

Resource impact: no timer, animation or polling is added. One `get_entity` request per slider
widget whose entity has no features loaded, once. Negligible.

## Migration Plan

None; the UI keeps no state. Verification target: the unit tests for the C++ logic, and the desktop
simulator (`UC_MODEL=DEV`) against the core-simulator for the screens and widgets. The number
screen's keys are `ButtonNavigation` keys and are pressed in the button simulator.

## Open Questions

None.
