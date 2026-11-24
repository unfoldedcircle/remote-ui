## ADDED Requirements

### Requirement: No-value placeholder
A value the UI does not know SHALL read "—" (em dash) without a unit: a value never reported, null or empty, the value of an unavailable entity and the value of an entity that is not loaded. "N/A" and "--" SHALL NOT be shown for such a value.

#### Scenario: Sensor without a value
- **WHEN** a temperature sensor item shows a sensor that has not reported a value
- **THEN** the value reads "—" without "°C"

#### Scenario: Unavailable sensor
- **WHEN** the sensor of a sensor item becomes unavailable
- **THEN** the value reads "—" and the red link-slash icon precedes the label

## MODIFIED Requirements

### Requirement: Entity tile content
An entity tile SHALL be 130 px high and show the entity icon (100 px) at the left, the name (up to two lines, elided) and a one-line state line. The icon SHALL be at full opacity when the entity counts as active and at 40 % otherwise: Button when Available or On; Switch and Light when On; Climate when not Off; Cover when Open; Macro, Sensor and Select always. A tile of an entity that is not enabled SHALL be shown at 50 % opacity with a ban icon in place of the entity icon. A red link-slash icon SHALL precede the state line when the integration state is known and not `connected`. The state line SHALL be: the state text for Button and Switch; the state text followed by the brightness as a rounded percentage of 255 for a Light that is On (e.g. "On 50%"); the state text followed by the current temperature and unit label for Climate (e.g. "Heat 21.5°C"); the state text followed by the position for Cover (e.g. "Open 40%"), and the state text alone as long as the cover has not reported a position; value, a space and unit for Sensor (value only for binary sensors); the current option for a Select that is On, otherwise its state text; nothing for Macro. A cover's state line SHALL be refreshed whenever the core reports a new `position`, including a report that carries no state. A light that was Unavailable or Unknown SHALL keep its brightness, so that when it is On again — for example after its integration reconnected — the percentage is shown again without a new brightness report. A climate entity's temperature SHALL be shown whatever its value, 0 included, with the unit of the entity from the moment the tile is shown; a climate entity with feature `current_temperature` SHALL show "—" in place of the temperature as long as it has not reported a numeric current temperature, or when it reports none (null or a non-numeric value), and a climate entity without that feature and without a reported temperature SHALL show the state text alone.

#### Scenario: Dimmed light
- **WHEN** a light is On with brightness 128
- **THEN** its state line reads "On 50%"

#### Scenario: Light turned off
- **WHEN** the core reports state `off` for a light
- **THEN** the state line reads "Off" and the brightness is reset to 0

#### Scenario: Cover position text
- **WHEN** the core reports only a new `position` for a cover without a state change
- **THEN** the percentage in the tile's state line is updated right away

#### Scenario: Closed cover tile
- **WHEN** a cover reports state `closed`
- **THEN** its tile icon is dimmed to 40 % and the on/off control of the same tile is off; an open cover shows both lit

#### Scenario: Light back from unavailable
- **WHEN** a light that was On with brightness 128 becomes Unavailable and is then reported On again without a new brightness
- **THEN** its state line reads "On 50%" again

#### Scenario: Climate at zero degrees
- **WHEN** a climate entity in state Heat reports a current temperature of 0 as its first value
- **THEN** its state line reads "Heat 0°C"

#### Scenario: Climate without a current temperature
- **WHEN** a climate entity with feature `current_temperature` in state Heat has not reported a current temperature yet, or reports `current_temperature` as null
- **THEN** its state line reads "Heat --"

### Requirement: Cover position
A cover with feature `position` SHALL show the position as a large "<n>%" and a slider from 0 to 100 in steps of 1 that follows the reported `position`; every change plays a Bump haptic, and releasing the slider SHALL send `cover.position` with `position`. Holding DPAD_UP for 300 ms SHALL raise the slider by 1 at once and then every 150 ms, switching to every 40 ms after the fifth step; DPAD_DOWN lowers it the same way. On release of the held key, `cover.position` with the new `position` SHALL be sent 500 ms later, and no open or close command is sent. The touch slider controls the position while the screen is open (see `touch-slider`). Until the cover has reported a position the readout SHALL read "—" without the percent sign; a cover without feature `position` never reports one. As soon as the user sets the position on screen, with the slider or with a held DPAD_UP / DPAD_DOWN, the readout SHALL show that value. A reported `position` SHALL be taken as a percentage and clamped to 0..100; a non-numeric value SHALL be ignored and the previous position kept.

#### Scenario: Hold to adjust
- **WHEN** DPAD_DOWN is held for about one second on a cover at 80 % with `position`
- **THEN** the slider decreases step by step, and 500 ms after release `cover.position` is sent with the shown value

#### Scenario: Hold without position feature
- **WHEN** DPAD_UP is held for one second on a cover without `position` but with `open`
- **THEN** `cover.open` is sent when the key is released, and nothing is sent when the cover has no `open` feature either

#### Scenario: Position never reported
- **WHEN** the screen of a cover that has never reported a position is opened
- **THEN** the readout reads "—", and it shows the value being set as soon as the user drags the slider

#### Scenario: Position outside the range
- **WHEN** the core reports `position` 140 for a cover
- **THEN** the cover is shown at 100 %

### Requirement: Sensor control screen
The sensor control screen SHALL show the value centred in large type (up to two lines, elided) over a dark gradient and SHALL offer no controls or key mappings besides closing. For battery, current, energy, humidity, power, temperature and voltage it SHALL show a heading "Battery", "Current", "Energy", "Humidity", "Power", "Temperature" or "Voltage" above "<value> <unit>"; a custom sensor shows "<value> <unit>" without heading; for both an empty value and the value of an unavailable sensor SHALL read "—" without the unit. A binary sensor SHALL show only its value text, shrunk horizontally to fit.

#### Scenario: No value yet
- **WHEN** a humidity sensor has not reported a value
- **THEN** its screen shows "Humidity" and "—"

### Requirement: Sensor widget on activity pages
A sensor item on an activity UI page SHALL show a one-line label and below it the value, both clipped to the item's grid area; the label is elided and the value uses up to two lines before eliding. The label SHALL be the entity's `custom_label` when the item's `show_label` is on and the label is set, otherwise the item's text. The value SHALL be "<value> <unit>", using `custom_unit` when set and the entity's unit otherwise; the unit SHALL be left out when `show_unit` is off. A red link-slash icon SHALL precede the label when the entity is not enabled. A missing or unavailable entity and an empty value SHALL show "—" without the unit. The widget SHALL update when the value or unit changes or the entity is loaded later.

#### Scenario: Custom label wins over item text
- **WHEN** a sensor item has text "Living" and `show_label` on, and the entity's `custom_label` is "Room temp"
- **THEN** the label reads "Room temp"

#### Scenario: Unit hidden
- **WHEN** `show_unit` is off for a sensor reporting 45 with unit %
- **THEN** the value reads "45 " without the unit

### Requirement: Select widget on activity pages
A select item on an activity UI page SHALL show a one-line label (the entity name when the item's `show_name` is on, otherwise the item's text), the current option below it (up to two lines, elided; the translated "None", dimmed, when no option is selected) and a chevron, clipped to the item's grid area. A red link-slash icon SHALL precede the label when the entity is not enabled; a missing entity SHALL show the item's text as label and "—" as option. Tapping the item SHALL open a full-screen "Select an option" list that takes the keys and behaves like the select control screen (row tap, DPAD_UP / DOWN / MIDDLE, DPAD_LEFT / RIGHT, PREV / NEXT), closing after a selection; BACK, HOME, the close icon or a tap outside SHALL close it without sending anything. The list slides and fades in and out in 300 ms.

#### Scenario: Choose from an activity page
- **WHEN** the user taps a select item and then taps an option
- **THEN** `select.select_option` is sent with that option and the list closes

#### Scenario: Stepping from the option list
- **WHEN** DPAD_LEFT is pressed in the option list of a select standing on its first option
- **THEN** `select.select_previous` is sent with `cycle` = true and the entity steps to the last option

#### Scenario: No option selected
- **WHEN** the select entity of an activity page item has no selected option
- **THEN** the item shows the translated "None" dimmed in place of the option
