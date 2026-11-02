## Why

The Core-API adds a number entity: a single numeric value an integration driver can read and set, such as an
amplifier's bass level, a fan speed or a charging current limit. The web-configurator places it on activity pages as
a number widget, binds a slider widget to it or to another numeric entity feature, and assigns it to the Remote 3
touch slider. The UI shows none of it today, and a slider or number item on an activity page is ignored.

The same Core-API revision defines how numeric values are formatted, for the number entity and for the sensor
entity, and lets the user override the number of decimal places and the number entity's representation in the
entity's `options.user`. The UI shows a sensor value exactly as reported today, so `1.499999999999997` reaches the
screen, and it ignores the sensor's `decimals` option.

## What Changes

- New entity type `number` with the options `min` / `max` (each may be `null`: unbounded), `step`, `decimals`,
  `unit` (default from the device class), `display_mode` and `readable`, the `value` attribute (`null`: no value),
  and the commands `set_value`, `increment` and `decrement`. Default icon `uc:input-numeric`; a Free-font fallback for
  it and for the former default `uc:slider`.
- A control screen for the number entity: a vertical slider or a stepper with decrement and increment buttons,
  chosen by the effective display mode; D-pad and volume keys step the value.
- The value set by the user is shown at once and replaced by the next reported value; without a report within 3 s
  the last known value returns, except for an entity with `readable: false`, which keeps the value sent.
- Activity pages render the `slider` item (bound to an entity feature: number value, light brightness and colour
  temperature, media-player volume, cover position, climate target temperature) and the `number` item (the number
  entity in its effective representation).
- The Remote 3 touch slider can control a bounded number entity's value, and a light's colour temperature when the
  activity names that feature.
- Sensor values: a numeric value is shown with the effective decimals (user override, then the `decimals` option),
  otherwise with at most 2 decimal places and no trailing zeros. Text values are shown as reported.
- An entity change event whose options became empty still updates the entity, so removing the last user override
  takes effect. The overrides are edited in the web-configurator; the UI does not offer editing them.
- A colour temperature slider, on the light screen and in a slider item, offers the positions of the light's
  `color_temperature_steps` option spread over 0..100, and sends the 0..100 value instead of the position.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `entity-management`: the supported types and default icons gain `number`; the device class fallback names its
  screen; option updates and user overrides are applied.
- `entity-detail-controls`: the sensor value format replaces "shown as reported"; the light's colour temperature
  slider follows `color_temperature_steps`; new requirements for the number entity, its control screen, the number
  widget and the slider widget.
- `activities`: activity UI pages render the `slider` and `number` items.
- `touch-slider`: a bounded number entity's `value` and a light's `color_temperature` are slider targets.

## Impact

- Hardware models: Remote Two and Remote 3 alike; the touch slider target is Remote 3 only (ADR 0008).
- Core-API: the number entity, its `entity_command`s and `entity_change` events, the `slider` and `number` user
  interface items, the `touch_slider` target `number.value`, the entity `options.user` overrides in entity change
  events, and the default icon `uc:input-numeric` for a number entity without a driver icon
  (core-api `doc/entities/entity_number.md`, `entity_sensor.md`, `README.md` "User overrides", `doc/ui-widgets.md`).
  The UI and the core ship together (ADR 0005); no fallback for an older core.
- Code: new `src/ui/entity/number.*` and `src/ui/entity/valueFormat.*`; `sensor.*`, `entity.*`,
  `entityController.*`, `entityScreens.cpp`, `src/core/core.cpp`, `src/core/structs.h`, `src/logging.*`; new QML
  `NumberWidget.qml`, `SliderWidget.qml`, `TouchSliderNumber.qml`, `entities/number/deviceclass/Number.qml`; changed
  `Activity.qml`, `TouchSlider.qml`, `entities/Base.qml`; `src/ui/entity/light.*` and
  `entities/light/Brightness.qml` for the colour temperature positions; new `TouchSliderColorTemperature.qml`.
- Resources: `resources/qrc/main.qrc`, `resources/icons/icon-fallback.json`.
- Tests: new `testNumberEntity`, `testSensorEntity` and `testLightEntity`; `testEntityScreens`, `testEntityController` and
  `testIconFont` extended.
- Translations: `en_US.ts`, the number states and two notifications, each with a translator comment.
- Third-party code and assets: none.
- Docs: `CHANGELOG.md`.
