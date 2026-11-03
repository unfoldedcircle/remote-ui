## Why

The UI shows a value it does not know in three different ways: "N/A" on the sensor screens, the
sensor and select items, the dock details and the system version, "--" for a cover position and a
climate device's current temperature, and "N/A %" with the unit for a sensor without a value. The
design system now has one placeholder for all of them: an em dash "—" (U+2014), without a unit.
The number entity change applies it to the slider and number items; this change applies it to the
existing screens and items.

## What Changes

- A value that is not known reads "—" without a unit, wherever it is shown today as "N/A" or "--":
  - the sensor control screens of every device class with a value and unit (a binary sensor always
    has a text) and the sensor button;
  - the sensor item: a missing entity, an unavailable entity and an empty value;
  - the select item: the option of a missing entity, whose label is now the item's text instead of
    "N/A";
  - the cover screens: the position before the first report, without the percent sign;
  - the climate screen and tile: the current temperature before the first report;
  - the dock details: an unknown connection type or firmware version;
  - the About and software update pages: the system version before the first update check.
- The sensor control screens and the sensor item show "—" for an unavailable sensor instead of its
  last value; the screen keeps its "Entity unavailable" overlay and the item its red link-slash
  icon.
- Unchanged: the faded look of an unavailable entity stays with the items that have it; the sensor
  and select items keep their current look, since the shared opacity tokens come with the number
  entity change.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `entity-detail-controls`: a new requirement "No-value placeholder" states the rule; "Entity tile
  content", "Cover position", "Sensor control screen", "Sensor widget on activity pages" and
  "Select widget on activity pages" read "—" instead of "N/A" or "--".
- `software-update`: "Version information" shows "—" before the first check.
- `docks`: "Dock details popup" shows "—" for an unknown connection type or firmware version.

## Impact

- Hardware models: Remote Two and Remote 3 alike; no hardware involved.
- Core-API: no message changes; the UI shows the same reported values.
- Code: QML of the sensor, select, cover, climate and dock screens and items; `climate.cpp` (the
  tile state line) and the initial system version in `softwareUpdate.h`.
- Fonts: Poppins and Space Mono, provided by the firmware, have the U+2014 glyph.
- Translations: the source text "N/A" ("Abbreviation for not available") is no longer used and is
  marked obsolete in `en_US.ts`; "—" is a symbol and not translated.
- Tests: the climate state line test expects "—".
- Third-party code and assets: none.
- Docs: `CHANGELOG.md`.
