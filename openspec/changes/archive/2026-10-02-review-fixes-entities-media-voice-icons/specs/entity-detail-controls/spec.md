## MODIFIED Requirements

### Requirement: Entity tile content
An entity tile SHALL be 130 px high and show the entity icon (100 px) at the left, the name (up to two lines, elided) and a one-line state line. The icon SHALL be at full opacity when the entity counts as active and at 40 % otherwise: Button when Available or On; Switch and Light when On; Climate when not Off; Cover when Open; Macro, Sensor and Select always. A tile of an entity that is not enabled SHALL be shown at 50 % opacity with a ban icon in place of the entity icon. A red link-slash icon SHALL precede the state line when the integration state is known and not `connected`. The state line SHALL be: the state text for Button and Switch; the state text followed by the brightness as a rounded percentage of 255 for a Light that is On (e.g. "On 50%"); the state text followed by the current temperature and unit label for Climate (e.g. "Heat 21.5°C"); the state text followed by the position for Cover (e.g. "Open 40%"), and the state text alone as long as the cover has not reported a position; value, a space and unit for Sensor (value only for binary sensors); the current option for a Select that is On, otherwise its state text; nothing for Macro. A cover's state line SHALL be refreshed whenever the core reports a new `position`, including a report that carries no state. A light that was Unavailable or Unknown SHALL keep its brightness, so that when it is On again — for example after its integration reconnected — the percentage is shown again without a new brightness report. A climate entity's temperature SHALL be shown whatever its value, 0 included, with the unit of the entity from the moment the tile is shown; a climate entity with feature `current_temperature` SHALL show "--" in place of the temperature as long as it has not reported a numeric current temperature, or when it reports none (null or a non-numeric value), and a climate entity without that feature and without a reported temperature SHALL show the state text alone.

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

### Requirement: Climate temperature unit and range
A climate entity SHALL use the unit from its option `temperature_unit` (`CELSIUS` or `FAHRENHEIT`) when valid; otherwise it SHALL use °F when the remote's unit system is US and °C for any other unit system, and SHALL follow later unit system changes. Defaults SHALL be 10–30 °C in steps of 0.5, or 50–86 °F in steps of 1. The options `min_temperature`, `max_temperature` and `target_temperature_step` SHALL override the defaults; a step of 0.09 or less, or a non-numeric value, SHALL be ignored. The selectable temperatures SHALL run from the maximum down to the minimum in steps with 0.1 precision; whole values are shown without decimals, others with one decimal. The tile's current temperature SHALL carry the unit label of the entity from the moment the entity is created, also for a value reported with the entity's creation, and SHALL switch to the new unit label as soon as the unit changes, without waiting for the next temperature report.

#### Scenario: US unit system without option
- **WHEN** a climate entity has no `temperature_unit` option and the unit system is US
- **THEN** the selectable range is 86 down to 50 in steps of 1 with label °F

#### Scenario: Custom step
- **WHEN** `target_temperature_step` is 0.1 with the Celsius defaults
- **THEN** the list offers 30, 29.9, 29.8 … 10

#### Scenario: Unit system changed
- **WHEN** the unit system changes from metric to US and the entity has no `temperature_unit` option
- **THEN** the range, step and unit label switch to the Fahrenheit values

#### Scenario: Fahrenheit entity on creation
- **WHEN** a climate entity with option `temperature_unit` FAHRENHEIT is created with a current temperature of 72
- **THEN** its tile shows "72°F" right away, not "72°C"

### Requirement: Climate target temperature
The climate control screen SHALL show the selectable temperatures as a vertical wheel with three visible values; the selected value is large with the unit label and coloured blue when the current temperature is above the target, red when below and white when equal, and white while the entity has no current temperature. With feature `current_temperature` it SHALL show "Current <temperature><unit>" with no decimals when the step is 1 and one decimal otherwise, and "Current --" while the entity has no current temperature (none reported yet, or reported as null or non-numeric). The wheel SHALL be positioned on the entity's target temperature 200 ms after opening (fading in) and whenever the core reports a new target; a target that is not one of the selectable temperatures — off the step grid or outside the range — SHALL position the wheel on the nearest selectable temperature. DPAD_UP or a tap on the upper area SHALL select the next higher value and DPAD_DOWN or a tap on the lower area the next lower value; the selection is sent 500 ms after the last such input. When a swipe of the wheel comes to rest the selection SHALL be sent immediately. The target SHALL be sent as `climate.target_temperature_c` or `climate.target_temperature_f`, matching the entity's unit, with parameter `temperature`. There SHALL be no control for a target temperature range, and `climate.target_temperature_range` and `climate.fan_mode` are never sent.

#### Scenario: Two presses
- **WHEN** a Celsius entity targets 21 and DPAD_UP is pressed twice within 500 ms
- **THEN** 22 is selected and one `climate.target_temperature_c` with `temperature` = 22 is sent 500 ms after the second press

#### Scenario: Target off the step grid
- **WHEN** the screen of a Celsius entity with step 0.5 opens while the target is 21.3, and DPAD_UP is pressed once
- **THEN** the wheel starts on 21.5, the nearest selectable temperature, and 22 is sent 500 ms after the press

#### Scenario: Target outside the range
- **WHEN** the screen of a Celsius entity with the default range opens while the target is 35
- **THEN** the wheel starts on 30, and DPAD_DOWN selects and sends 29.5

#### Scenario: No current temperature
- **WHEN** the screen of an entity with feature `current_temperature` opens before the entity has reported a current temperature
- **THEN** it reads "Current --" and the selected value is white

### Requirement: Binary sensor values
For a sensor with device class `binary`, the `unit` attribute SHALL carry the binary device class, and the `value` SHALL be shown as a translated text: the first text when the value is `on` (case-insensitive), the second for any other value. The pairs SHALL be: battery Normal/Low; battery_charging Charging/Not charging; carbon_monoxide and gas Detected/Clear; cold Cold/Normal; connectivity Connected/Disconnected; door and garage_door Opened/Closed; heat Hot/Normal; light Light detected/No light; lock Unlocked/Locked; moisture Wet/Dry; motion, occupancy, smoke, sound and vibration Detected/Clear; moving Moving/Not moving; opening and window Open/Closed; plug Plugged in/Unplugged; power On/Off; presence Home/Not home; problem Problem/Ok; running Running/Not running; safety Unsafe/Safe; tamper Tampering detected/Clear; update Update detected/Up-to-date; none or unknown classes On/Off. The value SHALL be kept as reported and the text SHALL be determined whenever it is shown, from that value, the current binary device class and the current language: a change of the display language SHALL update the text (500 ms after the change, with the state texts), and so SHALL a `unit` attribute that arrives after the value. A binary sensor that has not reported a value SHALL show no text.

#### Scenario: Garage door sensor
- **WHEN** a binary sensor with unit `garage_door` reports value `on`
- **THEN** its tile and screen show "Opened"

#### Scenario: Binary sensor without class
- **WHEN** a binary sensor has no `unit` attribute and reports `off`
- **THEN** it shows "Off"

#### Scenario: Language changed
- **WHEN** a binary sensor with unit `door` shows "Opened" and the display language is changed to German
- **THEN** the tile and the screen show the German text without the sensor reporting a new value

#### Scenario: Device class after the value
- **WHEN** a binary sensor reports value `on` first and its `unit` attribute `motion` afterwards
- **THEN** it shows "On" until the unit arrives and "Detected" from then on
