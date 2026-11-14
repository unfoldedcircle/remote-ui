## Context

### Current State Analysis (commit `b0020e9`, `main`)

- `SensorDeviceClass` has 9 values (`sensor.h:66`); `fromString()` maps every other device class to
  `Custom`. A sensor with device class `timestamp`, `date` or `duration` is a custom sensor today.
- `Sensor::getValue()` (`sensor.cpp:147`) knows two kinds of values: a number, formatted with the
  effective decimals, and a text, shown as reported. `getUnit()` returns the stored unit
  (`sensor.h:327`).
- The value and unit reach the screen in three places, all through the `value`, `unit` and
  `stateInfo` properties: the tile state line (`getStateInfo()`, `sensor.h:340`), the control
  screens (`sensor/deviceclass/*.qml`, "<value> <unit>") and the sensor item
  (`SensorWidget.qml:59`, which prefers the `custom_unit` option over the unit).
- The screen registry lists the 9 sensor device classes (`entityScreens.cpp:69-80`);
  `testEntityScreens` fails for a device class without an entry.
- A sensor entity knows the language it was created with and nothing about the time zone or the
  time format. The entity controller passes the language and the unit system on to the entities
  (`entityController.cpp:604-677`, `uiController.cpp:129-130`). The status bar clock converts the
  time with `QTimeZone(Config.timezone)` (`uiController.cpp:759`) and formats it as "hh:mm" or
  "hh:mm a" (`StatusBar.qml:124`).
- The app does not set a default `QLocale`; `ValueFormat` formats numbers with the default one.

### Constraints

- The value formats and the duration logic are specified in the sensor entity documentation of the
  core-api (`doc/entities/entity_sensor.md`, "Date, time and duration values" and "Duration
  format"). The duration logic is the one of the Home Assistant frontend (`formatDuration`), so that
  a value reads the same on the Remote and in Home Assistant.
- Qt 5.15 has no unit or duration formatter (`Intl.DurationFormat` in the reference).
- No periodic work the visible content does not need (`platform-constraints`).

## Goals / Non-Goals

**Goals:** a timestamp, a date and a duration read as a person expects, with the settings of the
Remote, in the tile, the control screen and the sensor item alike.

**Non-Goals:**

- A time relative to now ("in 3 hours"). It needs a timer per visible sensor and a text for every
  language; the documentation allows it and does not require it.
- A time span for a duration in seconds or milliseconds: the reference shows a number.
- The `duration` device class of the number entity.
- Headings ("Timestamp", "Duration") on the control screen: the entity name is the title.

## Decisions

### D1 — The formats are pure functions in `ValueFormat`

`timeSpan()`, `dateTime()` and `date()` take the value and the settings and return the text, or an
empty text when the value is not in the format. `Sensor::getValue()` falls back to the existing
number and text path then, so a value the UI cannot read is shown as reported (ADR 0018). The
functions are tested without an entity or QML (ADR 0009, ADR 0012).

### D2 — `value` and `unit` stay the only interface to QML

A duration that is shown as a time span returns an empty `unit`, so the tile, the 9 control
screens and the sensor item need no change and cannot disagree. `unitChanged` is emitted with a
new duration value and with new options, since both decide whether the unit is shown. The
`custom_unit` option is not read for the three device classes: the sensor item would append it to
"1h 30m". Alternative: a `formattedValue` property and a QML change in every place; rejected, more
places to keep in step for the same result.

### D3 — The three device classes use the screen of a custom sensor

The registry maps `Date`, `Duration` and `Timestamp` to `Custom.qml`: a value without a heading.
No new QML file.

### D4 — The duration follows the reference implementation step by step

Round to the effective decimals (a half is rounded up, as `Math.round` does), take the whole part
with `floor`, convert the rest to the next smaller unit with `floor`. A negative value with a rest
would read "-2h 30m" for -1.5 h; the reference refuses mixed signs and shows the number, and so
does the UI. The unit labels are four translated texts ("%1d", "%1h", "%1m", "%1s") joined with a
space, in place of the narrow CLDR unit patterns of the reference.

### D5 — Short date of the language, time as in the status bar

`QLocale(language).toString(date, QLocale::ShortFormat)` and the time as "hh:mm" or "hh:mm a". The
locale is built from the language code, since the app has no default locale. The short date fits
the 56 px value of the control screen on one or two lines. Alternative: the long format of the
reference ("November 15, 2026 at 5:23 AM"); rejected, it does not fit the tile and the sensor item.

### D6 — Language, time zone and time format reach the sensor through the entity controller

The UI controller passes `Config.timezone` and `Config.clock24h` to the entity controller, at
start-up and on every change; the entity controller emits `dateTimeFormatChanged` with the
language, and a sensor is connected to it when it is first used, as a climate entity is to the unit
system. A date or timestamp sensor emits `valueChanged` then. Alternative: reading `Config` in
`ValueFormat`; rejected, hidden global state that a test cannot set.

## Risks / Trade-offs

- [Risk] A binary fraction rounds down one unit: 2.3 h is 2 h and 17.99… min and reads "2h 17m" →
  the same as the reference; accepted to keep one logic. See Open Questions.
- [Risk] The short date has a two-digit year in most languages ("15.11.26") → readable for the
  near dates sensors report; the long format does not fit.
- [Risk] An unknown time zone name makes `QTimeZone` invalid → the system's local time is used
  instead of showing nothing.
- [Risk] The configuration arrives after the first sensor is shown → the entity controller emits
  the change, and the sensor updates its text.

Resource impact: none measurable. No timer, no polling, no animation and no new Qt module; one
string format per value change, and one signal connection per sensor entity in use.

## Migration Plan

Verification on the desktop simulator against the core-simulator with the Home Assistant
integration: a timestamp sensor (next backup), a duration sensor in hours and one in seconds, in
the tile, the control screen and a sensor item; then a change of the time zone, the time format and
the language while they are shown. No hardware involved; the time zone data on the device is the
one the status bar clock uses.

## Open Questions

- Should the UI correct the floating point artifact of the reference (2.3 h → "2h 17m" instead of
  "2h 18m")? It would be a deliberate deviation from the Home Assistant frontend.
