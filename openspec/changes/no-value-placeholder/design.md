## Context

### Current State Analysis (commit `aa8a8f2`, `main`)

- "N/A" (`qsTranslate("Abbreviation for not available", "N/A")`) is shown for:
  - an empty sensor value on the control screens of 8 device classes
    (`sensor/deviceclass/{Battery,Current,Custom,Energy,Humidity,Power,Temperature,Voltage}.qml`,
    e.g. `Humidity.qml:63`), followed by the unit: "N/A %";
  - an empty value of `BaseSensorButton.qml:23`;
  - a missing entity in `SensorWidget.qml:55` and, for label and option, in `SelectWidget.qml:43-44`;
  - an unknown connection type and firmware version in `docks/Info.qml:375,393`.
- "N/A" is the initial system version, `m_currentVersion` (`softwareUpdate.h:97`), shown on the
  About page and the software update page until the first update check. `m_newVersion` and
  `m_releaseNotes` start as "N/A" too, but are shown only while an update is available, after the
  check has set them.
- "--" is shown for a cover position before the first report (`cover/deviceclass/{Blind,Curtain,
  Garage,Window}.qml:175`, followed by "%"), for the current temperature on the climate screen
  (`Climate.qml:297-299`, "Current --") and in the climate tile state line (`climate.cpp:100`).
- An unavailable sensor keeps showing its last value on its screen, under the "Entity unavailable"
  overlay of `BaseDetail.qml`, and on its item, after the red link-slash icon.
- Tests: `testEntityController` checks the climate state line for "--"
  (`test_entity_controller.cpp:519,527,541`).

### Constraints

- The placeholder is a symbol, not a word: no translation, and the same in every language.
- The number entity change, not yet on `main`, adds the rule and the unavailable look to the design
  system with `ui` tokens of their own. This change adds no tokens, so the two do not overlap.

## Goals / Non-Goals

**Goals:** one placeholder, "—" without a unit, for every value the UI does not know.

**Non-Goals:** the faded unavailable look for the sensor and select items; a red mark for a missing
entity, which the UI cannot tell from one that is still loading, since a failed load is not
reported.

## Decisions

### D1 — "—" as a literal in the QML and C++ sources

Each place writes `"—"`. A shared constant would have to come from C++ to reach every QML file
and would add a dependency for one character. Alternative: a translated text like "N/A"; rejected,
a symbol needs no translation and translators would only copy it.

### D2 — No unit after the placeholder

"N/A %" and "--%" read as a value with a unit. The placeholder stands alone: the sensor screens and
item leave the unit out, the cover screens hide the percent sign.

### D3 — An unavailable sensor reads "—"

Its last value is not current. The screen keeps its overlay and the item its red icon; both now
read "—" underneath. The item re-evaluates when the entity's availability changes.

## Risks / Trade-offs

- [Risk] A firmware font without the glyph shows a box → Poppins and Space Mono, provided by the
  firmware, both have U+2014.
- [Risk] A select item of a missing entity without item text shows no label → its option reads "—",
  so the item does not look empty.

Resource impact: none; text only, plus one signal handler in the sensor item.

## Migration Plan

Verification in the simulator: a sensor without a value and an unavailable one, a cover that has
not reported a position, a climate device without a current temperature, and the dock details.
The fonts are the same on the device; no hardware involved.

## Open Questions

None.
