Four phases, one pull request each, stacked in this order: every phase adds a `CHANGELOG.md` entry,
and phases 1 and 2 share `SliderWidget.qml`, phases 2 and 4 `light.*`. Each phase carries its unit
tests and passes `make test`, `./cpplint.sh` and `./design-check.sh`.

## 1. Number entity, screen and stepper buttons (findings 1 to 6)

- [x] 1.1 `Base::commandFinished()`; `EntityController` reports an executed command and a command
      that failed for good to its entity.
- [x] 1.2 `Number`: `atMin` / `atMax`; no step towards a reached limit; a failed `set_value` ends
      the wait; a write-only number steps on an executed command.
- [x] 1.3 `Number.qml`: the stepper uses `atMin` / `atMax`; the slider snaps from `min` with
      `SliderStyle.snap`.
- [x] 1.4 `NumberWidget.qml`, `SliderWidget.qml` (`buttons` style): the button at a limit is
      disabled and dimmed.
- [x] 1.5 Unit tests: `test_number_entity.cpp` (limits, failed `set_value`, write-only steps),
      `test_entity_controller.cpp` (a failed `set_value` reaches the entity).
- [x] 1.6 `CHANGELOG.md`, "Unreleased", "Fixed".

## 2. Slider widget (findings 7 to 10)

- [x] 2.1 `SliderWidget.qml`: a feature missing from `features` is unsupported; features are
      fetched once when they are not loaded.
- [x] 2.2 `Light::colorKnown`; the slider widget uses it for `color: entity`.
- [x] 2.3 `SliderStyle::dragValue` clamps the start; the bar ignores a drag towards the end it is
      shown at.
- [x] 2.4 `vertical` set by `Activity.qml` from the item's `size`; the number widget passes it on.
- [x] 2.5 Unit tests: `test_slider_style.cpp` (drag from outside the range), `test_light_entity.cpp`
      (colour known).
- [x] 2.6 `CHANGELOG.md`, "Unreleased", "Fixed".

## 3. Activity pages (findings 11 and 12)

- [ ] 3.1 `SensorWidget.qml`, `SelectWidget.qml`: the unavailable look of the slider and number
      widgets, with the design system tokens.
- [ ] 3.2 `Activity`: `ready_check` is true unless it is the boolean `false`.
- [ ] 3.3 Unit test for `ready_check` `null`, missing and not a boolean.
- [ ] 3.4 `CHANGELOG.md`, "Unreleased", "Fixed".

## 4. Entity options (finding 13)

- [ ] 4.1 `Switch`, `Light`, `Climate`, `MediaPlayer`, `Sensor`: `applyOptions()` for the
      constructor and `updateOptions()`, with notify signals for the properties that follow.
- [ ] 4.2 Unit tests: an option update per type, and an update without the option.
- [ ] 4.3 `CHANGELOG.md`, "Unreleased", "Fixed".

## 5. Verification

- [ ] 5.1 `make macos` builds; no translation churn. The static aarch64 build runs in the Build
      workflow of each pull request.
- [ ] 5.2 Desktop simulator (`UC_MODEL=DEV`) against the core-simulator: the number screen's keys
      and slider at the limits, the number and slider widgets at a limit, a bar with a value
      outside its range, a 2 × 2 slider on a 6 × 6 grid, unavailable sensor, select, slider and
      number widgets side by side.
- [ ] 5.3 `openspec validate number-slider-widget-fixes --strict`.

## Workflow follow-up

- Archive the change after the last pull request is merged.
