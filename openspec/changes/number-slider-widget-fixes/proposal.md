## Why

The UI rewrite compared the number entity, the slider and number widgets and the entity options
with the core-api and with each other, and found thirteen places where the app at `b0020e9` does
something else than the core-api says or than another place in the app does
([#67](https://github.com/unfoldedcircle/remote-ui/issues/67)). Each finding was checked against the
code at `6d8e89a`; all thirteen hold.

## What Changes

Number screen and number entity:

- A step towards a limit the value has reached sends nothing: the d-pad and volume keys behave like
  the stepper button, which is disabled there.
- A `number.set_value` that fails ends the wait for the confirmation at once; the reported value
  shows again next to the error message instead of up to 3 s later.
- A write-only number (`readable` false) changes its shown value by one step for every `increment`
  / `decrement` the driver executed, no longer for every key press.
- The slider's positions count from `min`, with `max` as the last one, as on a slider widget.

Number and slider widgets:

- The stepper button of a number widget, and of a slider widget in the `buttons` style bound to a
  number, is disabled and dimmed at a limit.
- A slider widget whose feature is not in the entity's `features` is shown as unsupported.
- `color: entity` uses the light's colour only once the light reported `hue` and `saturation`.
- A bar whose value lies outside its range starts its drag from the nearest end.
- The orientation of a slider or number widget follows the item's `size` in grid cells.

Activity pages and entity options:

- An unavailable sensor or select widget is dimmed like an unavailable slider or number widget.
- A `ready_check` that is `null` or not a boolean counts as `true`.
- The options of an `entity_change` event are applied by every entity type that reads options.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `entity-detail-controls`: number steps at a limit, number value feedback after a failed command
  and for a write-only number, the positions of the number screen's slider, stepper buttons at a
  limit, slider widget feature check, entity colour, bar drag outside the range, widget
  orientation, the unavailable look of the sensor and select widgets.
- `activities`: the default of the `ready_check` option.
- `entity-management`: option updates for every entity type.

The requirements are ADDED: the number and slider requirements they refine are deltas of the
`number-entity` change, which is not archived yet, and a MODIFIED delta can only name a requirement
of the living specs.

## Impact

- Hardware models: Remote Two and Remote 3 alike (ADR 0008).
- Core-API: nothing new. `entity_command` and its result, `entity_change` with `options` and
  `features`, `get_entity`; all provided by the released core.
- Code: `src/ui/entity/` (`entity.h`, `entityController.cpp`, `number.*`, `light.*`,
  `sliderStyle.*`, `activity.cpp`, `switch.*`, `climate.*`, `mediaPlayer.*`, `sensor.*`),
  `src/qml/components/` (`NumberWidget.qml`, `SliderWidget.qml`, `SensorWidget.qml`,
  `SelectWidget.qml`, the number and activity screens).
- Tests: `test/ui/` number, slider style, light, entity controller and entity tests.
- Docs: `CHANGELOG.md`.
- Third-party code and assets: none.
