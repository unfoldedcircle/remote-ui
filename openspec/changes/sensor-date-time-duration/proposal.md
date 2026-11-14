## Why

The sensor entity has three new device classes for values a person does not read as raw data:
`timestamp`, `date` and `duration` ([core-api#104](https://github.com/unfoldedcircle/core-api/pull/104)).
The Home Assistant integration sends them
([integration-home-assistant#125](https://github.com/unfoldedcircle/integration-home-assistant/pull/125)).
The UI does not know these classes and shows such a sensor as a custom sensor with the raw value:
"2026-11-15T04:23:31+00:00" for the next backup, in UTC, or "1.5 h" for a remaining time. A driver
cannot format the value itself, since the time zone, the time format and the language are settings
of the Remote.

## What Changes

- The sensor device classes `timestamp`, `date` and `duration` are recognised. They use the control
  screen of a custom sensor: the value without a heading.
- A timestamp is shown as short date and time of the display language, in the configured time zone
  and with the time in the 12 or 24 hour format of the status bar clock: "15.11.26 05:23".
- A date is shown as short date of the display language: "15.11.26".
- A duration in days, hours or minutes is shown as a time span in the unit of the sensor and the
  next smaller unit, without the unit after it: 1.5 h reads "1h 30m". The logic is the one of the
  Home Assistant frontend, as the sensor entity documentation specifies. A duration in seconds or
  milliseconds is shown as a number with the unit, as before.
- The tile, the control screen and the sensor item show the same text, and it follows a change of
  the language, the time zone and the time format without a new value from the sensor.
- A value that is not in the format of its device class is shown as it is today: a text as
  reported, a number in the sensor value format with the unit.
- Not part of this change: a time relative to now ("in 3 hours"), which the documentation allows
  but does not require, and the `duration` device class of the number entity.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `entity-detail-controls`: "Sensor device classes and units" and "Sensor control screen" name the
  three device classes; new requirements "Sensor timestamp format", "Sensor date format", "Sensor
  duration format" and "Duration shown as a number".

## Impact

- Hardware models: Remote Two and Remote 3 alike; no hardware involved.
- Core-API: no new message or field. The `device_class` of an entity is a free text the core passes
  on, and the localization settings are the ones the UI already reads (`time_zone`,
  `time_format_24h`, `language_code`). A released core delivers all of it.
- Code: `valueFormat.*` (the three formats), `sensor.*`, `entityScreens.cpp`,
  `entityController.*` and `uiController.cpp` (time zone and time format for the sensor entities).
  No QML change and no new file.
- Translations: four short unit labels ("%1d", "%1h", "%1m", "%1s") in `en_US.ts`.
- Tests: `testSensorEntity`; `testEntityScreens` covers the new registry entries.
- Third-party code and assets: none. The time zone data is the one of the system, which the status
  bar clock already uses.
- Docs: `CHANGELOG.md`.
