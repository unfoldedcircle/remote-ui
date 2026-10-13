## MODIFIED Requirements

### Requirement: Inverted button behaviour setting
The UI settings SHALL offer the switch "Inverted button behavior", stored on the remote and off by default, described as "Inverts button functions on the main screen: short press to open the control screen, long press to quick toggle.". When off, a short DPAD_MIDDLE press (released before 800 ms) on the selected tile SHALL trigger the quick action and a long press (800 ms) SHALL open the control screen; when on, the two SHALL be swapped. The same swap SHALL apply to a closed group tile (toggle the group versus open it) and to the selected entity row of an open group. Touch gestures SHALL NOT be affected. Both presses SHALL behave the same on a tile placed directly on a page and on the same tile inside an open group, for an available entity and for an Unavailable one (see `entity-commands`).

#### Scenario: Default behaviour
- **WHEN** the setting is off and DPAD_MIDDLE is pressed briefly on a light tile
- **THEN** the light is toggled

#### Scenario: Inverted behaviour
- **WHEN** the setting is on and DPAD_MIDDLE is pressed briefly on a light tile
- **THEN** the light control screen opens, and holding DPAD_MIDDLE for 800 ms toggles the light instead

#### Scenario: Unavailable row in a group
- **WHEN** DPAD_MIDDLE is pressed short or long on the row of an open group whose entity state is Unavailable
- **THEN** no command is sent, no screen opens and the notification "<name> is unavailable" is shown
