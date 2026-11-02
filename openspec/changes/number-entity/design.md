## Context

### Current State Analysis (`aa8a8f2`)

- **Entity types.** `Base::Type` (`src/ui/entity/entity.h:36`) has 12 supported types; a `number` entity from the
  core becomes an unsupported entity with the `uc:warning` icon (`src/ui/entity/entity.cpp:73`), and the screen
  registry (`src/ui/entity/entityScreens.cpp:26-67`) has no screen for it.
- **Activity pages.** `parsePageItems` (`src/qml/components/entities/activity/deviceclass/Activity.qml:117-188`)
  renders text, icon, media_player, sensor and select; a `slider` or `number` item is logged as "Not implemented
  item type". Remote-entity pages render text and icon only (`remote/deviceclass/Remote.qml:30-63`) and are not
  changed here.
- **Touch slider.** `TouchSlider.qml:88-182` knows media player volume and seek, light brightness and cover
  position; a light's colour temperature is not a target. It waits for the entity's features before choosing (`TouchSlider.qml:108`).
- **Sensor values.** The value is stored as reported (`sensor.cpp:497`) and shown with `QVariant::toString()`
  (`sensor.h:324`, `sensor.h:344`), so a calculated value shows its floating point artifacts. The `decimals` option
  is parsed (`sensor.cpp:39`) but never read; the living spec says it "SHALL NOT affect what is shown".
- **Option updates.** `EntityController::onEntityChanged` passes options on only when the map is not empty
  (`entityController.cpp:777`). The core sends the complete entity on an update; when the last user override of an
  entity without driver options is removed, the options are an empty object and the update is dropped.
- **Free icon font.** `uc:input-numeric` and `uc:slider` are Pro-only and have no entry in
  `resources/icons/icon-fallback.json`, so a build with the embedded Free font would draw the placeholder.
- **Colour temperature.** The light screen's slider runs from 0 to the `color_temperature_steps` option (default
  100), shows the 0..100 attribute as a position and sends the position as `color_temperature`
  (`src/qml/components/entities/light/Brightness.qml:98`, `:230-233`, `:259`). Without the option the two agree;
  with `color_temperature_steps` 3 the slider sends 0..3 instead of 0, 50, 100.

### Constraints

- ADR 0005: the core owns the value. The UI never calculates a stepped value for a command; `increment` and
  `decrement` carry no `step`.
- ADR 0012: logic in C++, QML presents. The value format, the representation and the confirmation fallback are
  number entity properties.
- ADR 0019 / 0020: new QML outside the entity detail screens passes `./design-check.sh` and uses the colour tokens
  and type roles.
- ADR 0007: the activity page keeps its input idiom; widgets add no key handling.

## Goals / Non-Goals

**Goals:**

- The number entity on tiles, its control screen, the number and slider widgets and the Remote 3 touch slider, as
  the Core-API documents it.
- One value format for sensor and number entities, with the user overrides applied everywhere.

**Non-Goals:**

- Editing the user overrides: the web-configurator edits them, the remote only applies them.
- Showing a `duration` number as a time span; the spec allows it, the value is shown with its unit.
- D-pad control of a focused widget: the Core-API asks for it only on a screen that offers D-pad navigation of the
  user interface items. Activity pages have none, their D-pad keys switch pages and send the activity's button
  mapping.
- Slider and number items on remote-entity pages, which render no widget today.
- The `entity_available` event, which the UI does not receive.

## Decisions

### D1 — The number entity owns format, representation and feedback

`Number` exposes `formattedValue`, `decimals`, `unit`, `bounded` and `sliderRepresentation`, already resolved from
the driver options and `options.user`, and `value` / `hasValue` already resolved from the pending value. Tile,
screen, widgets and touch slider read the same properties. Alternative: resolve in each QML file; rejected, five
places would have to agree on precedence rules.

### D2 — One formatter for numeric entity values

`ValueFormat` (`src/ui/entity/valueFormat.*`) has `fixed(value, decimals)` and `compact(value)`, both rounding
before formatting so `-0.04` with one place is `0.0`, and `decimalPlaces(value)` with a relative tolerance so
`0.1` has one place. The current locale's decimal point is used, without group separator: values sit in narrow
widgets and tiles.

### D3 — The value set by the user is pending for 3 s

`setValue()` clamps to the limits that are set, sends `set_value` and keeps the value as pending. Any reported
`value` ends the wait; a single-shot timer of 3 s returns to the last reported value. With `readable: false` the
sent value becomes the known value. For a write-only entity `increment` / `decrement` change the shown value by
`step` within the range, because no report will ever come; this is for display only, the command still carries no
value, as the Core-API documents for a write-only entity.

### D4 — The number widget reuses the slider widget

The slider representation is the slider widget bound to `value` with `on_release`. The stepper is a small layout of
its own. Both refuse a command to an unavailable entity through `EntityController.mayCommandEntity()` with the same
notification as a tile.

### D5 — The slider widget resolves the feature in QML

The feature table (range, step, read and write per feature) stays in `SliderWidget.qml`, as the entity classes
expose the properties and setters it needs. Moving it to C++ would add a model class for one widget. `seek` is not a
slider widget feature; an unbounded number is shown as unsupported. The limits of the item narrow the range, they
cannot replace a missing limit.

### D6 — The touch slider uses the brightness gain for a number

A number's range has no natural size; the brightness gain (1.2 by default) gives a full sweep slightly more than the
range, like a light. No new setting.

### D7 — Options of a change event are applied when present

`core::Entity::optionsProvided` marks a change event that carried `options`, and the controller passes them on even
when empty. `Activity`, `Remote` and `VoiceAssistant` read only the keys present, so an empty map changes nothing
for them.

### D8 — The light maps colour temperature positions

`Light` exposes `colorTempPositionMax` (`color_temperature_steps` - 1, or 100 without the option or with more than
100 steps) and converts between a position and the 0..100 value in both directions, rounding to the nearest whole
number. The light screen, the slider widget and the touch slider use it; the command and the attribute stay 0..100.
The slider item's `min` / `max`, given in 0..100, are converted to positions too.

### D9 — Colour temperature on the touch slider: the brightness gain, at least 3 positions per swipe

A light's colour temperature is a touch slider target only when the activity names `color_temperature`; the default
feature of a light stays `dim`. The gesture works in positions, like the number entity in steps. A full swipe covers
the brightness gain times the highest position (`Light::colorTempPositionsPerSweep()`): with the default gain of 1.2,
120 of 0..100, the feel of the brightness slider. With 2 or 3 positions that rule would need 42 % to 83 % of the
slider length per change, so a swipe covers at least 3 positions: one change per third of the length, and every
position is reachable in one swipe from any start. Alternatives: a gain setting of its own, rejected as one more
setting for a rarely used target; a fixed travel per position, rejected because 101 positions would then need
several swipes.

## Risks / Trade-offs

- [Core-API event handling: an empty options object now reaches `updateOptions()`] → the three existing overrides
  only read keys that are present; `Sensor` and `Number` reset to the driver defaults, which is the intent.
- [A slow driver confirms after 3 s] → the value jumps back and then to the confirmed value; the Core-API asks
  drivers to confirm at once.
- [Registration: three new QML files and two C++ units] → `testEntityScreens` checks the screen against
  `main.qrc`; the widgets are loaded by URL and were checked with a build and a start of the app.
- [Static build] → no new Qt module; `QTimer` and `QLocale` are QtCore.

Resource impact: one single-shot timer per number entity, armed only after the user sets a value; no polling, no
animation. The widgets' 200 ms live timer runs only while a finger is down. Negligible against the budgets.

## Migration Plan

Verification target: the desktop simulator for the screens and widgets, against a core that provides the number
entity; the touch slider needs a Remote 3, for the number entity and for a light's colour temperature. No data migration: entities configured with the former default icon
`uc:slider` keep it and get a Free-font fallback.

## Open Questions

None.
