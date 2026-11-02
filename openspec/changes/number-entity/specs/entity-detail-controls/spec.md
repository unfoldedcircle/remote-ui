## MODIFIED Requirements

### Requirement: Sensor device classes and units
A sensor SHALL use device class `custom` (also for an empty or unknown class), `battery`, `current`, `energy`, `humidity`, `power`, `temperature`, `voltage` or `binary`. The default unit SHALL be % for battery and humidity, A for current, kWh for energy, W for power, °C for temperature and V for voltage, and empty for custom; a reported `unit` attribute SHALL replace it. The `value` SHALL be shown without unit conversion, formatted as the requirement "Sensor value format" describes. The options `native_unit`, `min_value` and `max_value` SHALL NOT affect what is shown; `custom_label` and `custom_unit` are only used by the sensor widget.

#### Scenario: Temperature in a US unit system
- **WHEN** a temperature sensor without `unit` attribute and without `decimals` option reports value 21.456 and the unit system is US
- **THEN** it is shown as "21.46 °C"

## ADDED Requirements

### Requirement: Sensor value format
A numeric sensor value SHALL be rounded for display only. With effective decimals, the user override `decimals` and otherwise the `decimals` option (0..6), it SHALL be shown with exactly that many decimal places. Without them it SHALL be shown with at most 2 decimal places and without trailing zeros. A text value SHALL be shown as reported. This applies to the tile, the control screen and the sensor widget.

#### Scenario: Decimals option
- **WHEN** a sensor with option `decimals` 1 reports 21
- **THEN** it is shown as "21.0"

#### Scenario: Floating point artifact
- **WHEN** a sensor without `decimals` reports 1.499999999999997
- **THEN** it is shown as "1.5"

#### Scenario: Text value
- **WHEN** a custom sensor with option `decimals` 1 reports the text "21.456"
- **THEN** it is shown as "21.456"

### Requirement: Number entity options
A number entity SHALL take its range from the options `min` (default 0) and `max` (default 100), either of which may be `null` for no limit, and its step size from `step` (default 1, at least 0.000001). The unit SHALL be the `unit` option, otherwise the default unit of the device class: dB for audio_level, % for battery and humidity, A for current, s for duration, kWh for energy, lx for illuminance, W for power, °C for temperature, V for voltage; no unit conversion is applied.

#### Scenario: Unit from the device class
- **WHEN** a number entity with device class `current` has no `unit` option
- **THEN** its value is shown with the unit "A"

#### Scenario: Temperature number in a US unit system
- **WHEN** a number entity with device class `temperature` reports 21 and the unit system is US
- **THEN** it is shown in °C

### Requirement: Number value format
A number value SHALL be shown with the effective decimals: the user override `decimals`, otherwise the `decimals` option (0..6), otherwise the largest number of decimal places of `step` and of the limits `min` and `max` that are set. The value SHALL always be shown with that many decimal places. A number entity without a value (`null`) SHALL show no value: its tile shows the state text and its screen and widgets "N/A".

#### Scenario: Decimals from the step size
- **WHEN** a number entity with `step` 0.5 and no `decimals` reports 21
- **THEN** it is shown as "21.0"

#### Scenario: Value unknown
- **WHEN** a number entity reports `value` null
- **THEN** its control screen shows "N/A"

### Requirement: Number representation
The effective display mode of a number entity SHALL be the user override `display_mode`, otherwise the `display_mode` option, otherwise `auto`. `slider` SHALL show a slider, `stepper` a stepper, and `auto` a slider for up to 256 positions `(max - min) / step` and a stepper for more. A number entity without `min` or without `max` SHALL always be shown as a stepper.

#### Scenario: Many positions
- **WHEN** a number entity with display mode `auto` has `min` 0, `max` 30 and `step` 0.1
- **THEN** it is shown as a stepper

#### Scenario: Unbounded slider
- **WHEN** a number entity with display mode `slider` has `max` null
- **THEN** it is shown as a stepper

### Requirement: Number control screen
The number control screen SHALL show the entity name, the value with its unit in large type, and the representation: a vertical slider whose value is sent with `number.set_value` when it is released, or decrement and increment buttons sending `number.decrement` and `number.increment` without a `step` parameter. DPAD_UP and VOLUME_UP SHALL send `number.increment`, DPAD_DOWN and VOLUME_DOWN `number.decrement`; PREV and NEXT SHALL set the value to `min` and `max` when these are set.

#### Scenario: Step with the d-pad
- **WHEN** DPAD_UP is pressed on a number control screen
- **THEN** `number.increment` is sent without parameters and the UI does not calculate the new value

#### Scenario: Slider released
- **WHEN** the user drags the slider of a number screen to 40 and releases it
- **THEN** `number.set_value` is sent with value 40

### Requirement: Number value feedback
After `number.set_value` the UI SHALL show the value sent at once and replace it with the next reported `value`; without a report within 3 s it SHALL show the last reported value again. For a number entity with option `readable` false the UI SHALL keep the value sent and, after `number.increment` or `number.decrement` on a known value, show it changed by `step` within the range.

#### Scenario: Driver does not confirm
- **WHEN** the user sets a number entity from 10 to 40 and the driver reports no value within 3 s
- **THEN** the value shown returns to 10

#### Scenario: Write-only number
- **WHEN** the user sets a number entity with `readable` false to 30
- **THEN** 30 stays shown

### Requirement: Number widget on activity pages
A number item on an activity UI page SHALL show the number entity in its effective representation. As a slider it SHALL behave like a slider item bound to the entity's value with `on_release` updates. As a stepper it SHALL show a one-line label, the value with its unit and decrement and increment buttons, vertically stacked when the item is taller than wide.

#### Scenario: Stepper item
- **WHEN** the user taps the increment button of a number item showing an amplifier's bass level
- **THEN** `number.increment` is sent without parameters

#### Scenario: Slider representation
- **WHEN** a number item shows a number entity whose effective display mode is `slider`
- **THEN** the item is a slider, and `number.set_value` is sent when it is released

### Requirement: Number widget label and availability
The label of a number item SHALL follow the label rules of the select widget. A red link-slash icon SHALL precede the label of an unavailable entity, and its buttons SHALL refuse the press with a notification like a tile.

#### Scenario: Unavailable entity
- **WHEN** the user taps a button of a number item whose entity is unavailable
- **THEN** no command is sent and a notification says the entity is unavailable

### Requirement: Slider widget on activity pages
A slider item SHALL show a label, the formatted value and a horizontal or vertical (taller than wide) slider for its entity feature: number `value`, light `dim` or `color_temperature`, media player `volume`, cover `position`, climate `target_temperature`, with the type's default when no feature is set. The item's `min` / `max` SHALL narrow the range. Other features, `seek` and an unbounded number SHALL show "N/A".

#### Scenario: Narrower range
- **WHEN** a volume slider item has `max` 60
- **THEN** the slider ends at volume 60

#### Scenario: Seek feature
- **WHEN** a slider item is bound to a media player's `seek` feature
- **THEN** it shows "N/A" and sends nothing

### Requirement: Slider widget updates
A slider item SHALL send its value when it is released, or every 200 ms while dragging and once more on release with `update_mode` `live`. It SHALL show the dragged value at once and the entity value again when the entity does not confirm within 3 s. A drag on an unavailable entity SHALL be refused with a notification like a tile.

#### Scenario: Live volume
- **WHEN** the user drags a live volume slider item for one second
- **THEN** volume commands are sent at most every 200 ms and once more on release

#### Scenario: Unavailable entity
- **WHEN** the user drags a slider item whose entity is unavailable
- **THEN** no command is sent, a notification says the entity is unavailable and the slider returns to the entity value
