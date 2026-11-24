One phase for the change and one for its verification; phase 2 depends on phase 1. Phase 1 is the
user-visible change and carries the `CHANGELOG.md` entry, in the same commit.

## 1. Placeholder "—"

- [x] 1.1 Sensor control screens of every device class with a value and unit, and `BaseSensorButton`:
      an empty value, and the value of an unavailable sensor, read "—" without the unit.
- [x] 1.2 `SensorWidget`: "—" for a missing or unavailable entity and an empty value; re-evaluate
      when the entity's availability changes.
- [x] 1.3 `SelectWidget`: a missing entity shows the item's text and the option "—".
- [x] 1.4 Cover screens: the unknown position reads "—" and hides the percent sign.
- [x] 1.5 Climate screen "Current —" and the tile state line in `climate.cpp`; the climate test in
      `testEntityController` expects "—" (it fails without 1.5).
- [x] 1.6 `docks/Info.qml`: unknown connection type and firmware version; `softwareUpdate.h`: the
      initial system version.
- [x] 1.7 `en_US.ts` regenerated; `CHANGELOG.md`, "Unreleased", "Changed". No new files, so no
      change to `remote-ui.pro`, a `.qrc` or a test CMakeLists.

## 2. Verification

- [x] 2.1 `make test` passes.
- [x] 2.2 Format the changed lines (`git clang-format`), `./cpplint.sh`, `./design-check.sh`, `qmllint`.
- [x] 2.3 `make linux`; no translation churn besides `en_US.ts`.
- [x] 2.4 `openspec validate no-value-placeholder --strict`; the deltas are synced into the living
      specs when the change is archived.
- [ ] 2.5 In the simulator: a sensor without a value and an unavailable one, a cover without a
      reported position, a climate device without a current temperature, the dock details.
