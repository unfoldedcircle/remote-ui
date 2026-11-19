## MODIFIED Requirements

### Requirement: Settings submenu
"Settings" SHALL list, in order: "Display & Brightness", "User interface", "Touch Slider", "Sound & Haptic", "Voice Control", "Power Saving", "Wifi & Bluetooth", "Web view" (only when the firmware provides the web view library), "Localization", "Administrator PIN", "Factory reset". Opening it re-requests the configuration from the core. The list scrolls when it overflows, keeping the selection between 15% and 85% of its height.

#### Scenario: Entry chosen
- **WHEN** an entry is chosen
- **THEN** its page loads on the third level

#### Scenario: Firmware with the web view
- **WHEN** the user opens "Settings" on a firmware that ships the web view library
- **THEN** "Web view" is listed directly below "Wifi & Bluetooth"

#### Scenario: Firmware without the web view
- **WHEN** the user opens "Settings" on a firmware without the library, or in the desktop simulator
- **THEN** "Localization" follows "Wifi & Bluetooth" and there is no "Web view" entry

## ADDED Requirements

### Requirement: Web view settings page
"Web view" SHALL show the core's global web view settings: "Color scheme" (Dark, Light), "Website data on close" (Keep,
Clear cache, Clear all), "Buttons" (a row that opens the buttons page) and the "Allow invalid certificates" switch.
Every row SHALL show its whole title, its value and a line that says what it does. A change SHALL be sent to the core
at once, only the changed setting, and the page SHALL show the core's answer; a change the core reports SHALL show at
once.

#### Scenario: Colour scheme changed
- **WHEN** the user picks "Light" in the "Color scheme" list
- **THEN** the UI sends `set_webview_cfg` with `color_scheme` `light` only, and the row reads "Light" once the core
  answered

#### Scenario: Change refused
- **WHEN** the core refuses the change of "Allow invalid certificates", for example with 403
- **THEN** a warning notification shows the core's message and the switch shows the setting as it was

#### Scenario: Change in the web configurator
- **WHEN** the website data setting is changed in the web configurator while the page is open
- **THEN** the "Website data on close" row shows the new value

#### Scenario: Whole titles
- **WHEN** the page opens on the 480 px panel with "Clear cache" as the website data setting
- **THEN** the row reads "Website data on close" in full, wrapped if needed, with "Clear cache" and "What closing a page
  deletes" under it

### Requirement: Web view buttons page
"Buttons" on the "Web view" page SHALL open, on the whole screen, the list of the buttons of the remote model that the
button mapping of the fullscreen web view names, in the order of the remote, each with its action under its name. BACK
and the arrow of its title bar SHALL return to the "Web view" page with the selection on "Buttons".

#### Scenario: The buttons of a Remote 3
- **WHEN** the user opens "Buttons" on a Remote 3
- **THEN** the list shows Back, the d-pad, OK, the volume, channel and media buttons, Stop, Record and Menu, and no
  colour button, HOME, POWER or VOICE

#### Scenario: Whole texts
- **WHEN** every button has its longest action, such as "Show or hide navigation" or "Sends MediaTrackPrevious", on the
  480 px panel
- **THEN** no title or action is cut off

### Requirement: Changing a button
OK or a tap on a button of the buttons page SHALL open the list of its actions: "Normal function" (`pass`), the web view
functions, and "Send key to page", which SHALL open the list of the keys. Both lists SHALL start on the current choice
and mark the default. A choice SHALL be sent to the core at once, with that button only, and the row SHALL show the
core's answer; a change the core reports SHALL show at once.

#### Scenario: A key for CHANNEL up
- **WHEN** the user picks "Send key to page" for "Channel up" and then `ChannelUp`
- **THEN** the UI sends `set_webview_cfg` with `buttons` `CHANNEL_UP` `{"action": "key", "key": "ChannelUp"}` only, and
  the row reads "Sends ChannelUp" once the core answered

### Requirement: Clearing website data from the settings
"Clear cache" and "Clear website data" on the "Web view" page SHALL clear the HTTP cache or all website data of every
web view, as the `clear_cache` and `clear_data` commands do, and a notification SHALL tell whether it worked. "Clear
website data" SHALL ask first, because it signs the user out of pages with a cookie login. With a web view library that
cannot clear website data, both buttons SHALL be disabled and the help text SHALL say so.

#### Scenario: Clear website data
- **WHEN** the user presses "Clear website data" and confirms the question with "Clear"
- **THEN** all website data of every web view is cleared, the shown pages reload and "Website data cleared" appears

#### Scenario: Question cancelled
- **WHEN** the user presses "Clear website data" and then Cancel
- **THEN** nothing is cleared

#### Scenario: Older web view library
- **WHEN** the page opens with a library older than 0.8.0
- **THEN** both buttons are disabled, the d-pad passes over them, and the help text reads "Clearing needs a newer
  firmware."

### Requirement: Web view test page
"Web view test" on the "Web view" page SHALL open the web view test page (address field, history and reload buttons,
the web page) on the whole screen, opaque: nothing of the "Web view" page SHALL show or react while it is open. BACK
and the arrow of its title bar SHALL close it and return to the "Web view" page with the selection on "Web view test".

#### Scenario: Test page closed with BACK
- **WHEN** the user opens "Web view test", loads a page and presses BACK
- **THEN** the "Web view" page shows again with "Web view test" selected, and the settings stay open

#### Scenario: Nothing of the page below
- **WHEN** the test page is open and the user taps between its controls, where the "Website data on close" row lies
  below
- **THEN** the test page shows its own background there, and no list opens
