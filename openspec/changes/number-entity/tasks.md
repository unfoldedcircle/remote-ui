One phase per commit; each user-visible phase carries its `CHANGELOG.md` entry. Phase 2 needs the formatter of
phase 1, phase 3 the number entity of phase 2, phase 4 the slider widget of phase 3, phase 5 the colour temperature positions of phase 4,
phase 6 the slider widget of phase 3.

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

## 4. Colour temperature positions

- [x] 4.1 `Light`: `colorTempPositionMax`, `colorTempToPosition()`, `colorTempFromPosition()` from
      `color_temperature_steps`.
- [x] 4.2 The light screen (`Brightness.qml`) and the slider widget show the nearest position and send its 0..100
      value; the slider item's `min` / `max` are converted to positions.
- [x] 4.3 `testLightEntity`; `CHANGELOG.md`, "Unreleased", "Fixed".

## 5. Colour temperature on the touch slider

- [x] 5.1 `Light::colorTempPositionsPerSweep()`: the brightness gain times the highest position, at least 3.
- [x] 5.2 `TouchSliderColorTemperature.qml`; the `color_temperature` case of a light in `TouchSlider.qml`, requiring
      the Color_temperature feature; the default feature of a light stays `dim`; `main.qrc` entry.
- [x] 5.3 `testLightEntity`: positions per sweep, every position survives the way through 0..100.
- [x] 5.4 `en_US.ts` regenerated; `CHANGELOG.md`, "Unreleased", "Added".

## 6. Slider styles

- [x] 6.1 `SliderStyle` (`src/ui/entity/sliderStyle.*`): the per-feature defaults, style, colour and fill resolution,
      relative drag, snap, positions, colour mix and contrast; QML singleton; `remote-ui.pro`.
- [x] 6.2 Slider tokens: `sliderGradientCool`, `sliderGradientWarm`, `sliderGradientHeat`, `sliderOnFill`, `sliderGrip`
      in `Colors`; the slider sizes, duration and opacities and `disabledOpacity` in `ui`; `docs/design-system.md`.
- [x] 6.3 `SliderWidget.qml`: the default, bar and buttons styles, colour and fill, separators and ticks, the text over
      the fill, relative drag in the bar, the unavailable and unsupported states.
- [x] 6.4 `style`, `color` and `fill` of the slider item in `Activity.qml`.
- [x] 6.5 `testSliderStyle`: defaults table, colour and fill resolution, relative drag, snap, discrete ranges;
      `testDesignTokens`: the slider colours and their contrast.
- [x] 6.6 `CHANGELOG.md`, "Unreleased", "Added".

## 7. Verification

- [x] 7.1 `make test`: all targets pass.
- [x] 7.2 `./cpplint.sh` clean, `./design-check.sh` 0 problems, `tools/icon-font.py check-mapping` consistent.
- [x] 7.3 `make linux` builds.
- [x] 7.4 `openspec validate number-entity --strict`.
- [ ] 7.5 In the simulator against a core with number entities: tile, screen in both representations, user
      overrides, slider and number items; a `null` value shows "N/A".
- [ ] 7.6 On a Remote 3: the touch slider on an activity whose target is a number entity.
- [ ] 7.7 A light with `color_temperature_steps` 3: the screen and a slider item offer three positions and send 0,
      50 and 100.
- [ ] 7.8 On a Remote 3: the touch slider on an activity whose target is a light's `color_temperature`, with and
      without `color_temperature_steps`; the same light with feature `default` is dimmed.
- [ ] 7.9 In the simulator against a core with the slider styles: each style horizontal and vertical, the colours and
      fills of the defaults table, a discrete range, an unavailable entity; a drag on a bar does not swipe the page.
- [ ] 7.10 On both remotes: the provisional slider tokens, the gradients and the text over a fill are readable.
