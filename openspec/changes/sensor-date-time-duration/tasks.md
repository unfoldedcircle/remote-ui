One phase for the change and one for its verification; phase 2 depends on phase 1. Phase 1 is the
user-visible change and carries the `CHANGELOG.md` entry, in the same commit.

## 1. Date, time and duration formats

- [x] 1.1 `ValueFormat`: `timeSpan()`, `dateTime()` and `date()`, returning an empty text for a
      value that is not in the format.
- [x] 1.2 `SensorDeviceClass`: `Date`, `Duration`, `Timestamp`; default unit s for a duration;
      `entityScreens.cpp` maps the three to the `Custom` screen.
- [x] 1.3 `Sensor`: `getValue()` uses the formats, `getUnit()` is empty for a time span,
      `custom_unit` is not read for the three device classes, `unitChanged` follows the value and
      the options of a duration, the unit labels follow a language change.
- [x] 1.4 `Sensor::onDateTimeFormatChanged()`; `EntityController` passes language, time zone and
      time format on; `uiController.cpp` connects `Config.timezone` and `Config.clock24h`.
- [x] 1.5 Unit tests in `testSensorEntity`: the three formats with the examples of the sensor entity
      documentation, and the sensor's value, unit and signals.
- [x] 1.6 `en_US.ts` regenerated with the four unit labels; `CHANGELOG.md`, "Unreleased", "Added".
      No new files, so no change to `remote-ui.pro`, a `.qrc` or a test CMakeLists.
- [x] 1.7 `ValueFormat::SECONDS_SPLIT_LIMIT`: a duration of 120 seconds or more in the unit `s` is
      shown as a time span; unit tests for the limit, the units and a negative value.

## 2. Verification

- [x] 2.1 `make test` passes.
- [x] 2.2 Format the changed lines (`git clang-format`), `./cpplint.sh`, `./design-check.sh`.
- [x] 2.3 `make macos`; no translation churn besides `en_US.ts`.
- [x] 2.4 `openspec validate sensor-date-time-duration --strict`; the deltas are synced into the
      living specs when the change is archived.
- [ ] 2.5 In the simulator: a timestamp sensor, a duration sensor in hours and one in seconds, in
      the tile, the control screen and a sensor item; a change of the time zone, the time format
      and the language while they are shown.
