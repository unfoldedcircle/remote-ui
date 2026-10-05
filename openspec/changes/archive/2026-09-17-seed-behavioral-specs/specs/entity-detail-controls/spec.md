## ADDED Requirements

### Requirement: Opening an entity control screen
Tapping an entity tile outside its icon SHALL open the entity's control screen when the entity is enabled and the page is not in edit mode. The screen SHALL be chosen by entity type and device class, slide up from the bottom in 300 ms, and request the entity from the core again when it opens so that it shows current data. While a control screen is already shown, another one SHALL NOT be opened on the same level.

#### Scenario: Tap on a light tile
- **WHEN** the user taps the name area of an enabled light tile
- **THEN** the light control screen slides up
- **AND** the entity is fetched from the core and its attributes are updated from the response

#### Scenario: Entity not enabled
- **WHEN** the user taps the tile of an entity that is not enabled
- **THEN** no control screen opens

### Requirement: Closing an entity control screen
A short or long press on BACK or HOME, or a tap on the close icon (×, top right, touch area enlarged by 20 px), SHALL close the control screen. It slides down in 300 ms and the screen underneath is shown again.

#### Scenario: BACK closes
- **WHEN** a switch control screen is open and BACK is pressed
- **THEN** the screen slides down and the page with the tiles is shown

### Requirement: Control screen title bar
Every entity control screen SHALL show an 80 px title bar with the entity icon (70 px) and name, and at the right a status cluster: a WiFi icon when WiFi is disconnected (struck through in red) or the signal is none or weak; the battery level when "show battery everywhere" is on (percentage text while charging or when the percentage setting is on, a bolt while charging, the bar red when the battery is low); and a spinning indicator while a command for the entity is in progress (see `entity-commands`). A red link-slash icon SHALL appear left of the close icon when the entity's integration state is known and not `connected`.

#### Scenario: Integration disconnected
- **WHEN** a control screen is open and the entity's integration reports a state other than `connected`
- **THEN** the red link-slash icon is shown next to the close icon

### Requirement: Unavailable entity on a control screen
While the entity state is Unavailable, the control screen SHALL cover everything below the close icon with an 85 % black overlay that swallows touches, and after 1 s show a red ban icon (120 px) with "Entity unavailable". The screen SHALL ignore all physical keys in this state, BACK and HOME included; only the close icon closes it. The overlay SHALL disappear as soon as the state changes away from Unavailable.

#### Scenario: Entity becomes unavailable while the screen is open
- **WHEN** the core reports state `unavailable` for the shown entity
- **THEN** the overlay covers the controls, and 1 s later "Entity unavailable" is shown
- **AND** pressing BACK does nothing, tapping × closes the screen

### Requirement: Entity tile content
An entity tile SHALL be 130 px high and show the entity icon (100 px) at the left, the name (up to two lines, elided) and a one-line state line. The icon SHALL be at full opacity when the entity counts as active and at 40 % otherwise: Button when Available or On; Switch and Light when On; Climate when not Off; Cover when Closed; Macro, Sensor and Select always. A tile of an entity that is not enabled SHALL be shown at 50 % opacity with a ban icon in place of the entity icon. A red link-slash icon SHALL precede the state line when the integration state is known and not `connected`. The state line SHALL be: the state text for Button and Switch; the state text followed by the brightness as a rounded percentage of 255 for a Light that is On (e.g. "On 50%"); the state text followed by the current temperature and unit label for Climate (e.g. "Heat 21.5°C"); the state text followed by the position for Cover (e.g. "Open 40%"); value, a space and unit for Sensor (value only for binary sensors); the current option for a Select that is On, otherwise its state text; nothing for Macro.

#### Scenario: Dimmed light
- **WHEN** a light is On with brightness 128
- **THEN** its state line reads "On 50%"

#### Scenario: Light turned off
- **WHEN** the core reports state `off` for a light
- **THEN** the state line reads "Off" and the brightness is reset to 0

#### Scenario: Cover position text
- **WHEN** the core reports only a new `position` for a cover without a state change
- **THEN** the percentage in the tile's state line is updated with the next state change, not immediately

### Requirement: Tile quick action
Tapping the icon of an entity tile, or triggering the quick action with DPAD_MIDDLE on the selected tile, SHALL perform the type's quick action: Button sends `button.push`; Switch sends `switch.toggle`, only if it has feature `on_off` or `toggle`; Light toggles (see "Light control pages"); Cover sends `cover.close` when Open and `cover.open` when Closed, only if it has both features `open` and `close`; Macro shows the run progress overlay (see `activities`) and sends `macro.run`; Select sends `select.select_next` with `cycle` = true. Climate and Sensor tiles SHALL have no quick action.

#### Scenario: Cover moving
- **WHEN** the icon of a cover in state Opening is tapped
- **THEN** no command is sent

#### Scenario: Switch without features
- **WHEN** the icon of a switch with neither `on_off` nor `toggle` is tapped
- **THEN** no command is sent

#### Scenario: Select tile
- **WHEN** the icon of a select tile is tapped
- **THEN** `select.select_next` is sent with `cycle` = true

### Requirement: Inverted button behaviour setting
The UI settings SHALL offer the switch "Inverted button behaviour", stored on the remote and off by default, described as "Inverts button functions on the main screen: short press to open the control screen, long press to quick toggle.". When off, a short DPAD_MIDDLE press (released before 800 ms) on the selected tile SHALL trigger the quick action and a long press (800 ms) SHALL open the control screen; when on, the two SHALL be swapped. The same swap SHALL apply to a closed group tile (toggle the group versus open it) and to the selected entity row of an open group. Touch gestures SHALL NOT be affected.

#### Scenario: Default behaviour
- **WHEN** the setting is off and DPAD_MIDDLE is pressed briefly on a light tile
- **THEN** the light is toggled

#### Scenario: Inverted behaviour
- **WHEN** the setting is on and DPAD_MIDDLE is pressed briefly on a light tile
- **THEN** the light control screen opens, and holding DPAD_MIDDLE for 800 ms toggles the light instead

#### Scenario: Unavailable row in a group
- **WHEN** DPAD_MIDDLE is pressed short or long on the row of an open group whose entity state is Unavailable
- **THEN** nothing happens

### Requirement: Tile edit menu
A long press on an entity tile SHALL open a menu titled with the entity name that offers "Rename", "Change icon" (showing the entity's current icon) and "Remove". Renaming and icon changes behave as specified in `entity-management`; "Remove" takes the entity off the page, or out of its group when the tile is inside a group, and saves the page (see `pages`). For a restricted profile the long press SHALL show the warning notification "Profile is restricted" instead of the menu.

#### Scenario: Restricted profile
- **WHEN** the current profile is restricted and the user long-presses a tile
- **THEN** "Profile is restricted" is shown and no menu opens

### Requirement: Entity states and state texts
Each type SHALL recognise exactly these states: Button — Unavailable, Unknown, Available, On; Switch and Light — Unavailable, Unknown, On, Off; Climate — Unavailable, Unknown, Off, Heat, Cool, Heat/Cool (`heat_cool`), Fan, Auto; Cover — Unavailable, Unknown, Opening, Open, Closing, Closed; Sensor and Select — Unavailable, Unknown, On; Macro — Unavailable, Unknown, Running, Error, Completed. The `state` value SHALL be matched case-insensitively. A value that matches none of them SHALL be ignored and the previous state kept. The state text SHALL be translated, falling back to the untranslated state name, and SHALL be refreshed 500 ms after the display language changes. Until the first valid state arrives the state text is empty. A macro SHALL re-publish every reported state, even an unchanged one.

#### Scenario: Unknown state value
- **WHEN** the core reports state `jammed` for a cover that is Closed
- **THEN** the cover stays Closed

#### Scenario: Upper-case state
- **WHEN** the core reports state `HEAT` for a climate entity
- **THEN** the entity is in state Heat

### Requirement: Button control screen
The button control screen SHALL show one large square button that highlights while pressed. Tapping it, DPAD_MIDDLE and POWER SHALL send `button.push`. The only device class is `button`.

#### Scenario: POWER on a button screen
- **WHEN** POWER is pressed on a button control screen
- **THEN** `button.push` is sent

### Requirement: Switch control screen
A switch SHALL use device class `switch` or `outlet`; an empty or unknown class SHALL fall back to `switch`, and `outlet` gets its own socket-style button face. The screen SHALL show a large square button filled white while the switch is On, and above it a large "On"/"Off" text only when the entity has feature `toggle`. Tapping the button, DPAD_MIDDLE and POWER SHALL send `switch.toggle` regardless of the entity's features.

#### Scenario: Switch with only on_off
- **WHEN** a switch with feature `on_off` but not `toggle` is opened
- **THEN** no On/Off text is shown, and tapping the button still sends `switch.toggle`

### Requirement: Light control pages
When the light control screen opens it SHALL build its pages from the features: an On/Off page when the light lacks `dim`, a brightness page when it has `dim`, and a colour page when it has `color`, in that order. Pages SHALL be switched by horizontal swipe or DPAD_LEFT / DPAD_RIGHT, with page dots when there is more than one page. The On/Off page shows a large "On"/"Off" text and a large button filled white while On. Tapping that button, DPAD_MIDDLE and POWER SHALL toggle the light: `light.toggle` when the entity has feature `toggle`, otherwise `light.off` when On and `light.on` in any other state. The touch slider controls the brightness while the screen is open (see `touch-slider`).

#### Scenario: Light without toggle feature
- **WHEN** DPAD_MIDDLE is pressed on the screen of a light in state Unknown that lacks `toggle`
- **THEN** `light.on` is sent

#### Scenario: Colour temperature without dim
- **WHEN** a light has `color_temperature` but not `dim`
- **THEN** only the On/Off page is shown and the colour temperature cannot be changed

### Requirement: Light brightness and colour temperature
The brightness page SHALL show a vertical brightness slider from 0 to 255 in steps of 1, labelled with the rounded percentage (0–100, no unit sign). When the light also has `color_temperature`, a colour temperature slider from 0 to the entity option `color_temperature_steps` (default 100) in steps of 1, labelled with the raw value, SHALL be shown next to it and both take half the width. Every value change SHALL play a Bump haptic. Releasing a slider SHALL send `light.on` with `brightness` or `color_temperature` respectively. The sliders follow the entity's reported values.

#### Scenario: Drag brightness
- **WHEN** the user drags the brightness slider to 191 and releases it
- **THEN** the label reads 75 and `light.on` is sent with `brightness` = 191

### Requirement: Light keys on the control screen
On the brightness and colour pages DPAD_UP / DPAD_DOWN SHALL raise / lower the brightness by 1 when the light has `dim`. VOLUME_UP / VOLUME_DOWN SHALL raise / lower the brightness by 1 and CHANNEL_UP / CHANNEL_DOWN the colour temperature by 1, but only when the light has both `dim` and `color_temperature`. The new value SHALL be sent as `light.on` with `brightness` or `color_temperature` 500 ms after the last such key press. These key handlers are not restricted to the visible page and stay active while the entity is Unavailable or a popup is open.

#### Scenario: Holding DPAD_UP
- **WHEN** DPAD_UP is held on a dimmable light's screen and released
- **THEN** the brightness rises by 1 per key repeat and a single `light.on` with the final `brightness` is sent 500 ms after the last repeat

#### Scenario: Volume keys on a dim-only light
- **WHEN** VOLUME_UP is pressed on a light with `dim` but without `color_temperature`
- **THEN** nothing changes

### Requirement: Light colour wheel
The colour page SHALL show a 440 px hue/saturation wheel at full value with a 60 px picker, and below it a horizontal brightness slider from 0 to 255 labelled with the rounded percentage. Dragging the picker SHALL update its colour live and keep it inside the wheel's circle (radius 220 px). Releasing the picker SHALL send `light.on` with `hue` (0–359, never negative; 0 when the colour has no hue) and `saturation` (0–255). The picker SHALL be placed on the light's current colour 200 ms after the page is shown and whenever the core reports a hue or saturation change, except within 300 ms after the user released it; the position is the first wheel point whose red, green and blue each differ from the colour by less than 10, and the centre for white, black or no match. Releasing the brightness slider SHALL send `light.on` with `brightness`.

#### Scenario: Fast drag past the rim
- **WHEN** the user drags the picker quickly beyond the edge of the wheel and releases it
- **THEN** the picker stays on the rim of the circle and the hue sent is between 0 and 359

#### Scenario: Picker reflects external change
- **WHEN** the colour is changed from another client while the colour page is shown
- **THEN** the picker moves to the new colour

### Requirement: Climate temperature unit and range
A climate entity SHALL use the unit from its option `temperature_unit` (`CELSIUS` or `FAHRENHEIT`) when valid; otherwise it SHALL use °F when the remote's unit system is US and °C for any other unit system, and SHALL follow later unit system changes. Defaults SHALL be 10–30 °C in steps of 0.5, or 50–86 °F in steps of 1. The options `min_temperature`, `max_temperature` and `target_temperature_step` SHALL override the defaults; a step of 0.09 or less, or a non-numeric value, SHALL be ignored. The selectable temperatures SHALL run from the maximum down to the minimum in steps with 0.1 precision; whole values are shown without decimals, others with one decimal. The tile's current temperature label is fixed when the temperature is reported, so a value reported while the entity is created carries °C even for a Fahrenheit entity until the next report.

#### Scenario: US unit system without option
- **WHEN** a climate entity has no `temperature_unit` option and the unit system is US
- **THEN** the selectable range is 86 down to 50 in steps of 1 with label °F

#### Scenario: Custom step
- **WHEN** `target_temperature_step` is 0.1 with the Celsius defaults
- **THEN** the list offers 30, 29.9, 29.8 … 10

#### Scenario: Unit system changed
- **WHEN** the unit system changes from metric to US and the entity has no `temperature_unit` option
- **THEN** the range, step and unit label switch to the Fahrenheit values

### Requirement: Climate target temperature
The climate control screen SHALL show the selectable temperatures as a vertical wheel with three visible values; the selected value is large with the unit label and coloured blue when the current temperature is above the target, red when below and white when equal. With feature `current_temperature` it SHALL show "Current <temperature><unit>" with no decimals when the step is 1 and one decimal otherwise. The wheel SHALL be positioned on the entity's target temperature 200 ms after opening (fading in) and whenever the core reports a new target. DPAD_UP or a tap on the upper area SHALL select the next higher value and DPAD_DOWN or a tap on the lower area the next lower value; the selection is sent 500 ms after the last such input. When a swipe of the wheel comes to rest the selection SHALL be sent immediately. The target SHALL be sent as `climate.target_temperature_c` or `climate.target_temperature_f`, matching the entity's unit, with parameter `temperature`. There SHALL be no control for a target temperature range, and `climate.target_temperature_range` and `climate.fan_mode` are never sent.

#### Scenario: Two presses
- **WHEN** a Celsius entity targets 21 and DPAD_UP is pressed twice within 500 ms
- **THEN** 22 is selected and one `climate.target_temperature_c` with `temperature` = 22 is sent 500 ms after the second press

### Requirement: Climate mode and fan menus
When the entity has any of the features `on_off`, `heat` or `cool`, the climate screen SHALL show a mode label at the bottom (the current mode, or "Mode" when Unavailable or Unknown) with a green dot; tapping it or pressing GREEN SHALL open the "Mode" menu. The menu SHALL list "Off" (`on_off` and not Off; sends `climate.off`), "Heat" (`heat` and not Heat), "Cool" (`cool` and not Cool) and "Auto" (both `heat` and `cool`, listed even when already Auto); Heat, Cool and Auto send `climate.hvac_mode` with `hvac_mode` HEAT, COOL or AUTO. When the entity has feature `fan`, a "Fan" label with a yellow dot SHALL be shown (each label taking half the width when both exist); tapping it or pressing YELLOW SHALL open a "Fan" menu that has no entries. DPAD_MIDDLE and POWER SHALL do nothing on the climate screen.

#### Scenario: Switch to cooling
- **WHEN** a heating entity with `heat`, `cool` and `on_off` shows the mode menu and "Cool" is chosen
- **THEN** `climate.hvac_mode` is sent with `hvac_mode` = COOL
- **AND** the menu had offered Off, Cool and Auto

#### Scenario: Entity without mode features
- **WHEN** GREEN is pressed on the screen of a climate entity with only `target_temperature`
- **THEN** no menu opens and no mode label is shown

### Requirement: Cover device classes
A cover SHALL be shown with the screen for its device class: `blind` (also for `shade` and for an empty or unknown class), `curtain`, `garage`, and `window` (also for `door` and `gate`). The screens SHALL behave the same and differ only in how the position is drawn: blind slats, a garage door with four panels, a window pane, or two curtain halves moving together. The title icon SHALL use the entity icon. The curtain screen's position display is only set when the core reports a changed position, so it shows 0 % until then.

#### Scenario: Gate cover
- **WHEN** a cover with device class `gate` is opened
- **THEN** the window screen is shown

### Requirement: Cover open and close
A cover without feature `position` SHALL show its state as "Open", "Closed" or "Unknown" (Opening, Closing and Unavailable also read "Unknown") and the buttons "Close" (enabled when Open or Unknown, sends `cover.close`) and "Open" (enabled when Closed or Unknown, sends `cover.open`); a disabled button is drawn at 50 % opacity. A cover with feature `stop` SHALL show a "Stop" label at the bottom that sends `cover.stop`, and DPAD_MIDDLE SHALL send `cover.stop`. On every cover screen a short press on DPAD_UP (released within 300 ms) SHALL send `cover.open` and a short press on DPAD_DOWN `cover.close`, regardless of the `open` and `close` features. Tilt features SHALL NOT be offered.

#### Scenario: Cover moving
- **WHEN** a cover without `position` is in state Closing
- **THEN** both Open and Close buttons are disabled and the state reads "Unknown"

#### Scenario: Short press
- **WHEN** DPAD_UP is pressed and released within 300 ms on a cover screen
- **THEN** `cover.open` is sent

### Requirement: Cover position
A cover with feature `position` SHALL show the position as a large "<n>%" and a slider from 0 to 100 in steps of 1 that follows the reported `position`; every change plays a Bump haptic, and releasing the slider SHALL send `cover.position` with `position`. Holding DPAD_UP for 300 ms SHALL raise the slider by 1 at once and then every 150 ms, switching to every 40 ms after the fifth step; DPAD_DOWN lowers it the same way. On release of the held key, `cover.position` with the new `position` SHALL be sent 500 ms later, and no open or close command is sent. The touch slider controls the position while the screen is open (see `touch-slider`).

#### Scenario: Hold to adjust
- **WHEN** DPAD_DOWN is held for about one second on a cover at 80 % with `position`
- **THEN** the slider decreases step by step, and 500 ms after release `cover.position` is sent with the shown value

#### Scenario: Hold without position feature
- **WHEN** DPAD_UP is held for one second on a cover without `position`
- **THEN** `cover.open` is sent when the key is released

### Requirement: Sensor device classes and units
A sensor SHALL use device class `custom` (also for an empty or unknown class), `battery`, `current`, `energy`, `humidity`, `power`, `temperature`, `voltage` or `binary`. The default unit SHALL be % for battery and humidity, A for current, kWh for energy, W for power, °C for temperature and V for voltage, and empty for custom; a reported `unit` attribute SHALL replace it. The `value` SHALL be shown as reported, without rounding and without unit conversion. The options `native_unit`, `decimals`, `min_value` and `max_value` SHALL NOT affect what is shown; `custom_label` and `custom_unit` are only used by the sensor widget.

#### Scenario: Temperature in a US unit system
- **WHEN** a temperature sensor without `unit` attribute reports value 21.456 and the unit system is US
- **THEN** it is shown as "21.456 °C"

### Requirement: Binary sensor values
For a sensor with device class `binary`, the `unit` attribute SHALL carry the binary device class, and the `value` SHALL be shown as a translated text: the first text when the value is `on` (case-insensitive), the second for any other value. The pairs SHALL be: battery Normal/Low; battery_charging Charging/Not charging; carbon_monoxide and gas Detected/Clear; cold Cold/Normal; connectivity Connected/Disconnected; door and garage_door Opened/Closed; heat Hot/Normal; light Light detected/No light; lock Unlocked/Locked; moisture Wet/Dry; motion, occupancy, smoke, sound and vibration Detected/Clear; moving Moving/Not moving; opening and window Open/Closed; plug Plugged in/Unplugged; power On/Off; presence Home/Not home; problem Problem/Ok; running Running/Not running; safety Unsafe/Safe; tamper Tampering detected/Clear; update Update detected/Up-to-date; none or unknown classes On/Off. The text SHALL be determined when the value is reported.

#### Scenario: Garage door sensor
- **WHEN** a binary sensor with unit `garage_door` reports value `on`
- **THEN** its tile and screen show "Opened"

#### Scenario: Binary sensor without class
- **WHEN** a binary sensor has no `unit` attribute and reports `off`
- **THEN** it shows "Off"

### Requirement: Sensor control screen
The sensor control screen SHALL show the value centred in large type (up to two lines, elided) over a dark gradient and SHALL offer no controls or key mappings besides closing. For battery, current, energy, humidity, power, temperature and voltage it SHALL show a heading "Battery", "Current", "Energy", "Humidity", "Power", "Temperature" or "Voltage" above "<value> <unit>"; a custom sensor shows "<value> <unit>" without heading; for both an empty value SHALL read "N/A". A binary sensor SHALL show only its value text, shrunk horizontally to fit.

#### Scenario: No value yet
- **WHEN** a humidity sensor has not reported a value
- **THEN** its screen shows "Humidity" and "N/A %"

### Requirement: Sensor widget on activity pages
A sensor item on an activity UI page SHALL show a one-line label and below it the value, both clipped to the item's grid area; the label is elided and the value uses up to two lines before eliding. The label SHALL be the entity's `custom_label` when the item's `show_label` is on and the label is set, otherwise the item's text. The value SHALL be "<value> <unit>", using `custom_unit` when set and the entity's unit otherwise; the unit SHALL be left out when `show_unit` is off. A red link-slash icon SHALL precede the label when the entity is not enabled. An unknown entity SHALL show "N/A". The widget SHALL update when the value or unit changes or the entity is loaded later.

#### Scenario: Custom label wins over item text
- **WHEN** a sensor item has text "Living" and `show_label` on, and the entity's `custom_label` is "Room temp"
- **THEN** the label reads "Room temp"

#### Scenario: Unit hidden
- **WHEN** `show_unit` is off for a sensor reporting 45 with unit %
- **THEN** the value reads "45 " without the unit

### Requirement: Select control screen
The select control screen SHALL list the entity's `options` as rows of 120 px (up to two lines each, elided) with the current option highlighted and scrolled to the centre on opening and whenever `current_option` changes. Tapping a row SHALL send `select.select_option` with `option` and close the screen. DPAD_UP / DPAD_DOWN SHALL move the highlight and DPAD_MIDDLE SHALL send the highlighted option and close the screen. DPAD_RIGHT SHALL send `select.select_next` with `cycle` = true, DPAD_LEFT `select.select_previous` with `cycle` = false, NEXT `select.select_last` and PREV `select.select_first`, each closing the screen. An empty `current_option` SHALL be shown as "None". After a display language change the tile's state line SHALL show the state text until the next `current_option` report.

#### Scenario: Pick with the d-pad
- **WHEN** the current option is "HDMI 1" and the user presses DPAD_DOWN then DPAD_MIDDLE
- **THEN** `select.select_option` is sent with the option below "HDMI 1" and the screen closes

#### Scenario: Previous does not wrap
- **WHEN** DPAD_LEFT is pressed on a select screen
- **THEN** `select.select_previous` is sent with `cycle` = false and the screen closes

### Requirement: Select widget on activity pages
A select item on an activity UI page SHALL show a one-line label (the entity name when the item's `show_name` is on, otherwise the item's text), the current option below it (up to two lines, elided; "None" when empty) and a chevron, clipped to the item's grid area. A red link-slash icon SHALL precede the label when the entity is not enabled; an unknown entity SHALL show "N/A" for label and option. Tapping the item SHALL open a full-screen "Select an option" list that takes the keys and behaves like the select control screen (row tap, DPAD_UP / DOWN / MIDDLE, DPAD_LEFT / RIGHT, PREV / NEXT), closing after a selection; BACK, HOME, the close icon or a tap outside SHALL close it without sending anything. The list slides and fades in and out in 300 ms.

#### Scenario: Choose from an activity page
- **WHEN** the user taps a select item and then taps an option
- **THEN** `select.select_option` is sent with that option and the list closes

### Requirement: Macro control screen
The macro control screen SHALL show one large square button that highlights while pressed. Tapping it, DPAD_MIDDLE and POWER SHALL show the run progress overlay (see `activities`) and send `macro.run`. The UI SHALL offer no way to send `macro.stop`.

#### Scenario: Run from the screen
- **WHEN** POWER is pressed on a macro control screen
- **THEN** the progress overlay appears and `macro.run` is sent
