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

### D10 — Slider styles: the per-feature defaults and the drag arithmetic in C++

The slider item's `style`, `color` and `fill` resolve in `SliderStyle` (`src/ui/entity/sliderStyle.*`), a QML
singleton: one table holds each feature's `auto` colour, gradient and `auto` fill, and the functions resolve a value
against it, fall back to the default for an unknown value (ADR 0018), and compute the relative drag, the snap to a
position and the number of positions. `testSliderStyle` covers them, as the project has no QML tests (ADR 0009, ADR
0012). The table names colour roles, `Colors` holds their values. The feature range table stays in QML (D5).
Alternative: a JavaScript singleton; rejected, it could not be unit tested.

### D11 — Slider tokens in `Colors` and `ui`

The slider design is not final, so every colour, size, radius, duration and opacity of the slider item is a token: the
gradient ends, the colour over a fill and the grip in `Colors`, the sizes, the duration and the opacities as `ui`
constants, documented in `docs/design-system.md` section 7. Fill, track, focus and text reuse the existing roles
(`textPrimary`, `surface`, `surfaceRaised`, `textSecondary`). The dark end of the brightness gradient is
`surfaceRaised`. The unavailable entity's 15 % and 50 % opacities are a documented exception to "emphasis by token".

### D12 — Absolute press in the default style, relative drag in the bar

The default style keeps the press-to-position of today's slider. A bar fills the whole item, so a touch anywhere on it
would set a value; it moves the value by the dragged distance instead, past the platform's drag threshold, and a tap
changes nothing. Both take the touch from the page swipe (`preventStealing`). A range of at most 10 positions follows
the finger between the positions and snaps on release; a larger one snaps at once. The widget draws its own track for
both styles instead of the Qt Quick Controls slider, which cannot drag relatively.

### D13 — Text over the fill: drawn twice, the second copy clipped to the fill

A bar draws its label, value and icons once in their colours and once in the colour over the fill, clipped to the fill
(or to the cursor band, or the whole track for a cursor on a gradient). The colour over the fill is `sliderOnFill` or
`textPrimary`, whichever contrasts more with the fill at its middle. Alternative: an opacity mask shader; rejected, a
layer per item costs texture memory on the device.

### D14 — A brightness button steps one percent

The Core-API defines one step of the buttons style as one step of the shown value. Brightness is shown in percent but
has the range 0..255 with step 1, so a button changes it by one percent (2.55, rounded). The other features step by
their step; a number sends `increment` / `decrement`.

The D-pad focus ring of a slider item (3 px `focusRing`) is not drawn: activity pages give no widget the focus
(Non-Goals). It follows with D-pad navigation of the user interface items.

### D15 — No value reads "—"

A value that is not known, of a number without a value, an unavailable entity or a feature the item cannot show,
reads "—" (U+2014) without a unit; "N/A" and "--" are not used. It is a symbol, not a translated text. The
unavailable look, the red mark and its 15 % and 50 % opacities, are `ui` tokens of their own, shared by the slider
and number items. An unsupported feature gets the mark too. An entity that is not loaded reads "—" without the mark:
the entity controller reports no failed load, so a missing entity cannot be told from one that is still loading.

### D16 — The number item's display mode and step

The number item's own `display_mode` is resolved by the entity, `Number::isSliderRepresentationFor()`, next to the
user override, the option and the auto rule, so the precedence is one unit-tested function; the control screen asks
for `auto` and keeps the entity's representation. The widget resolves it again when the options change, since a
method call in a QML binding does not follow them. The item's `step` goes to the core as the `step` parameter of
`increment` and `decrement`; the core and the driver calculate the value, and only a write-only number shows it
changed by that step, clamped to the range and not rounded to the entity's step. Activity pages give no widget the
D-pad focus (D14), so DPAD_LEFT / DPAD_RIGHT keep switching pages and the step applies to the buttons.

## Risks / Trade-offs

- [Core-API event handling: an empty options object now reaches `updateOptions()`] → the three existing overrides
  only read keys that are present; `Sensor` and `Number` reset to the driver defaults, which is the intent.
- [A slow driver confirms after 3 s] → the value jumps back and then to the confirmed value; the Core-API asks
  drivers to confirm at once.
- [Registration: three new QML files and two C++ units] → `testEntityScreens` checks the screen against
  `main.qrc`; the widgets are loaded by URL and were checked with a build and a start of the app.
- [Static build] → no new Qt module; `QTimer` and `QLocale` are QtCore.
- [A touch that starts on a slider item no longer swipes the page] → on a page filled with sliders the user swipes from
  another item or uses DPAD_LEFT / DPAD_RIGHT; the Core-API asks for this.
- [Provisional slider tokens] → each is one value in `Colors` or `ui`.

Resource impact: one single-shot timer per number entity, armed only after the user sets a value; no polling, no
animation. The widgets' 200 ms live timer runs only while a finger is down. A bar draws its few texts and icons
twice and the default track animates its thickness for 200 ms on a press; no layers, shaders or images. Negligible
against the budgets.

## Migration Plan

Verification target: the desktop simulator for the screens and widgets, against a core that provides the number
entity; the touch slider needs a Remote 3, for the number entity and for a light's colour temperature. No data migration: entities configured with the former default icon
`uc:slider` keep it and get a Free-font fallback.

## Open Questions

None.
