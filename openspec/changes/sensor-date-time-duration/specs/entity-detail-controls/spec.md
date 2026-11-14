## MODIFIED Requirements

### Requirement: Sensor device classes and units
A sensor SHALL use device class `custom` (also for an empty or unknown class), `battery`, `current`, `energy`, `humidity`, `power`, `temperature`, `voltage`, `date`, `duration`, `timestamp` or `binary`. The default unit SHALL be % for battery and humidity, A for current, kWh for energy, W for power, °C for temperature, V for voltage and s for duration, and empty for custom, date and timestamp; a reported `unit` attribute SHALL replace it. The `value` SHALL be shown without unit conversion, formatted as the requirement "Sensor value format" describes, and for a date, duration or timestamp sensor as the requirements "Sensor date format", "Sensor duration format" and "Sensor timestamp format" describe. The options `native_unit`, `min_value` and `max_value` SHALL NOT affect what is shown; `custom_label` and `custom_unit` are only used by the sensor widget, and `custom_unit` SHALL NOT be used for a date, duration or timestamp sensor.

#### Scenario: Temperature in a US unit system
- **WHEN** a temperature sensor without `unit` attribute and without `decimals` option reports value 21.456 and the unit system is US
- **THEN** it is shown as "21.46 °C"

#### Scenario: Duration without a unit
- **WHEN** a duration sensor without `unit` attribute reports value 90
- **THEN** it is shown as "90 s"

### Requirement: Sensor control screen
The sensor control screen SHALL show the value centred in large type (up to two lines, elided) over a dark gradient and SHALL offer no controls or key mappings besides closing. For battery, current, energy, humidity, power, temperature and voltage it SHALL show a heading "Battery", "Current", "Energy", "Humidity", "Power", "Temperature" or "Voltage" above "<value> <unit>"; a custom, date, duration or timestamp sensor shows "<value> <unit>" without heading; for both an empty value and the value of an unavailable sensor SHALL read "—" without the unit. A binary sensor SHALL show only its value text, shrunk horizontally to fit.

#### Scenario: No value yet
- **WHEN** a humidity sensor has not reported a value
- **THEN** its screen shows "Humidity" and "—"

#### Scenario: Timestamp sensor
- **WHEN** the screen of a timestamp sensor is opened
- **THEN** it shows the formatted date and time without a heading

## ADDED Requirements

### Requirement: Sensor timestamp format
A `timestamp` sensor value that is a date and time in RFC 3339 format with a UTC offset SHALL be shown as the short date of the display language and the time, converted to the configured time zone. The time SHALL be shown as in the status bar clock: in the 12 or 24 hour format and without seconds. The text SHALL follow a change of the language, the time zone and the time format without a new value. Any other value SHALL be shown as a sensor of class `custom` shows it.

#### Scenario: Time zone of the Remote
- **WHEN** a timestamp sensor reports "2026-11-15T04:23:31+00:00", the language is German (Switzerland), the time zone Europe/Zurich and the time format 24 hours
- **THEN** it is shown as "15.11.26 05:23"

#### Scenario: Time format is changed
- **WHEN** the user switches the time format to 12 hours while a timestamp sensor is shown
- **THEN** the sensor shows the time in the 12 hour format without reporting a new value

#### Scenario: Value without UTC offset
- **WHEN** a timestamp sensor reports "2026-11-15T04:23:31"
- **THEN** it is shown as "2026-11-15T04:23:31"

### Requirement: Sensor date format
A `date` sensor value in the format `YYYY-MM-DD` SHALL be shown as the short date of the display language, without a time zone conversion, and SHALL follow a change of the language without a new value. Any other value SHALL be shown as a sensor of class `custom` shows it.

#### Scenario: German date
- **WHEN** a date sensor reports "2026-11-15" and the language is German (Switzerland)
- **THEN** it is shown as "15.11.26"

#### Scenario: Not a date
- **WHEN** a date sensor reports "next week"
- **THEN** it is shown as "next week"

### Requirement: Sensor duration format
A numeric `duration` sensor value in the unit `d`, `h` or `min` SHALL be shown as a time span: the whole part of the value in the unit of the sensor and the rest, rounded down, in the next smaller unit (hours, minutes, seconds), which SHALL be left out when it is zero. With effective decimals the value SHALL first be rounded to that many decimal places. Each part SHALL carry a translated short unit label, and no unit SHALL follow the time span. The value SHALL NOT be converted to a larger unit.

#### Scenario: Hours and minutes
- **WHEN** a duration sensor with unit `h` reports 1.5
- **THEN** it is shown as "1h 30m" without a unit after it

#### Scenario: No larger unit
- **WHEN** a duration sensor with unit `h` reports 93
- **THEN** it is shown as "93h"

#### Scenario: Days and hours
- **WHEN** a duration sensor with unit `d` reports 3.9
- **THEN** it is shown as "3d 21h"

#### Scenario: Minutes and seconds
- **WHEN** a duration sensor with unit `min` reports 90.5
- **THEN** it is shown as "90m 30s"

#### Scenario: Rounded with decimals
- **WHEN** a duration sensor with unit `h` and effective decimals 0 reports 2.75
- **THEN** it is shown as "3h"

#### Scenario: Negative whole value
- **WHEN** a duration sensor with unit `h` reports -2
- **THEN** it is shown as "-2h"

### Requirement: Duration shown as a number
A `duration` sensor value SHALL be shown as the requirement "Sensor value format" describes, followed by the unit, when it is not shown as a time span: a value in the unit `s`, `ms` or any other unit, a negative value with a fractional part, and a text value. The unit SHALL appear and disappear with the value, without a new `unit` attribute.

#### Scenario: Seconds
- **WHEN** a duration sensor with unit `s` reports 673492
- **THEN** it is shown as "673492 s"

#### Scenario: Negative value with a fraction
- **WHEN** a duration sensor with unit `h` that shows "2h" reports -1.5
- **THEN** it is shown as "-1.5 h"
