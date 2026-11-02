One phase per commit; each user-visible phase carries its `CHANGELOG.md` entry. Phase 2 needs the formatter of
phase 1, phase 3 the number entity of phase 2.

## 1. Sensor value format

- [x] 1.1 `ValueFormat` (`src/ui/entity/valueFormat.*`): `decimalsOption`, `decimalPlaces`, `fixed`, `compact`;
      registered in `remote-ui.pro` and the test CMakeLists.
- [x] 1.2 `Sensor::getValue()` formats a numeric value with the effective decimals; `Sensor::updateOptions()`
      follows `decimals` and `options.user.decimals`.
- [x] 1.3 `core::Entity::optionsProvided`: empty options of a change event reach the entity.
- [x] 1.4 `testSensorEntity`: formatter, sensor format, override, empty options.
- [x] 1.5 `CHANGELOG.md`, "Unreleased", "Changed".

## 2. Number entity

- [x] 2.1 `Number` (`src/ui/entity/number.*`): options, device class units, effective decimals and display mode,
      pending value with 3 s fallback, `readable: false`; `lcNumber`; `remote-ui.pro`.
- [x] 2.2 Type `Number`, default icon `uc:input-numeric`, controller creation, features and QML enums, screen
      registry for every device class.
- [x] 2.3 `entities/number/deviceclass/Number.qml` (slider or stepper), the tile case in `entities/Base.qml`.
- [x] 2.4 Touch slider: `TouchSliderNumber.qml`, the number case in `TouchSlider.qml` for a bounded number.
- [x] 2.5 `main.qrc` entries; Free fallbacks `input-numeric` → `hashtag`, `slider` → `sliders`.
- [x] 2.6 `testNumberEntity`; `testEntityScreens`, `testEntityController` and `testIconFont` extended.
- [x] 2.7 `en_US.ts` regenerated with `lupdate`, the new texts filled with their English source.
- [x] 2.8 `CHANGELOG.md`, "Unreleased", "Added".

## 3. Slider and number widgets

- [x] 3.1 `SliderWidget.qml`: entity feature target without `seek`, unbounded number unsupported, number format,
      design tokens.
- [x] 3.2 `NumberWidget.qml`: slider widget or stepper by the effective display mode.
- [x] 3.3 `slider` and `number` items in `Activity.qml`; `main.qrc` entries.
- [x] 3.4 `en_US.ts` regenerated, `CHANGELOG.md`, "Unreleased", "Added".

## 4. Verification

- [x] 4.1 `make test`: all targets pass.
- [x] 4.2 `./cpplint.sh` clean, `./design-check.sh` 0 problems, `tools/icon-font.py check-mapping` consistent.
- [x] 4.3 `make linux` builds.
- [x] 4.4 `openspec validate number-entity --strict`.
- [ ] 4.5 In the simulator against a core with number entities: tile, screen in both representations, user
      overrides, slider and number items; a `null` value shows "N/A".
- [ ] 4.6 On a Remote 3: the touch slider on an activity whose target is a number entity.
