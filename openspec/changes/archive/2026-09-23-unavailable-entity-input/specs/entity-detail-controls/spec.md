## MODIFIED Requirements

### Requirement: Opening an entity control screen
Tapping an entity tile outside its icon SHALL open the entity's control screen when the entity is available and the page is not in edit mode. The screen SHALL be chosen by entity type and device class, slide up from the bottom in 300 ms, and request the entity from the core again when it opens so that it shows current data. While a control screen is already shown, another one SHALL NOT be opened on the same level. A tap, or the d-pad press that opens the screen, on an entity that is Unavailable SHALL open nothing and SHALL show the warning notification "<name> is unavailable"; while the remote is waking up the screen SHALL open normally (see `entity-commands`). A tile of an entity the UI does not know SHALL open nothing and show no notification. When the UI has no screen for the entity — a type it implements none for (a voice assistant, or an entity type only a newer core knows, which is kept as an unsupported entity) or a device class whose screen is missing — the screen SHALL close again by itself, with no empty screen left on display, and the failed screen SHALL be logged. The UI SHALL stay operable afterwards: the next entity, activity or settings page SHALL open normally, both for a screen opened from a page or a group and for one opened on top of an activity.

#### Scenario: Tap on a light tile
- **WHEN** the user taps the name area of an enabled light tile
- **THEN** the light control screen slides up
- **AND** the entity is fetched from the core and its attributes are updated from the response

#### Scenario: Entity not enabled
- **WHEN** the user taps the tile of an entity that is not enabled
- **THEN** no control screen opens and the notification "<name> is unavailable" is shown

#### Scenario: Entity without a screen
- **WHEN** the user opens a voice assistant entity, or an entity of a type this UI does not implement
- **THEN** the screen closes again immediately, the page underneath stays usable and the missing screen is logged

#### Scenario: Another screen after a failed one
- **WHEN** an entity without a screen was opened and the user then opens a light, an activity or a settings page
- **THEN** that screen opens normally

#### Scenario: Entity without a screen from an activity
- **WHEN** an entity of the activity's device list has no screen and the user opens it
- **THEN** it closes again and the activity screen stays as it was, and further entities of the list still open

### Requirement: Unavailable entity on a control screen
While the entity state is Unavailable, the control screen SHALL cover everything below the close icon with an 85 % black overlay that swallows touches, and after 1 s show a red ban icon (120 px) with "Entity unavailable". The screen SHALL ignore every physical key that would send a command in this state, and SHALL raise no notification for such a key because the overlay already says why nothing happens. BACK and HOME SHALL still close the screen, on a short and on a long press, as SHALL a tap on the close icon. On a screen that maps BACK or HOME to a command of its own (the media player screens), those keys SHALL close the screen while the entity is Unavailable instead of sending the command. The overlay SHALL disappear as soon as the state changes away from Unavailable, and the command keys SHALL work again from that moment; while the remote is waking up they keep working (see `entity-commands`).

#### Scenario: Entity becomes unavailable while the screen is open
- **WHEN** the core reports state `unavailable` for the shown entity
- **THEN** the overlay covers the controls, and 1 s later "Entity unavailable" is shown
- **AND** pressing DPAD_UP sends nothing, while BACK or HOME closes the screen and tapping × closes it too

#### Scenario: Entity comes back while the screen is open
- **WHEN** the core reports a state other than `unavailable` for the entity whose screen is open
- **THEN** the overlay disappears and the screen's command keys act again

### Requirement: Light keys on the control screen
On the brightness and colour pages DPAD_UP / DPAD_DOWN SHALL raise / lower the brightness by 1 when the light has `dim`. VOLUME_UP / VOLUME_DOWN SHALL raise / lower the brightness by 1 and CHANNEL_UP / CHANNEL_DOWN the colour temperature by 1, but only when the light has both `dim` and `color_temperature`. The new value SHALL be sent as `light.on` with `brightness` or `color_temperature` 500 ms after the last such key press. These key handlers SHALL act only on the feature page that is on screen, so that one key press changes one value once even though the neighbouring feature pages stay loaded. They SHALL do nothing while the entity is Unavailable, unless the remote is waking up (see `entity-commands`), and they stay active while a popup is open on top of the screen.

#### Scenario: Holding DPAD_UP
- **WHEN** DPAD_UP is held on a dimmable light's screen and released
- **THEN** the brightness rises by 1 per key repeat and a single `light.on` with the final `brightness` is sent 500 ms after the last repeat

#### Scenario: Volume keys on a dim-only light
- **WHEN** VOLUME_UP is pressed on a light with `dim` but without `color_temperature`
- **THEN** nothing changes

#### Scenario: One key press on a light with two feature pages
- **WHEN** DPAD_UP is pressed once while the brightness page of a light that also has a colour page is on screen
- **THEN** the brightness rises by 1 and exactly one `light.on` is sent 500 ms later

#### Scenario: Keys on an unavailable light
- **WHEN** DPAD_UP, VOLUME_UP or CHANNEL_UP is pressed while the light is Unavailable and no resume is pending
- **THEN** no value changes and no command is sent

### Requirement: Inverted button behaviour setting
The UI settings SHALL offer the switch "Inverted button behaviour", stored on the remote and off by default, described as "Inverts button functions on the main screen: short press to open the control screen, long press to quick toggle.". When off, a short DPAD_MIDDLE press (released before 800 ms) on the selected tile SHALL trigger the quick action and a long press (800 ms) SHALL open the control screen; when on, the two SHALL be swapped. The same swap SHALL apply to a closed group tile (toggle the group versus open it) and to the selected entity row of an open group. Touch gestures SHALL NOT be affected. Both presses SHALL behave the same on a tile placed directly on a page and on the same tile inside an open group, for an available entity and for an Unavailable one (see `entity-commands`).

#### Scenario: Default behaviour
- **WHEN** the setting is off and DPAD_MIDDLE is pressed briefly on a light tile
- **THEN** the light is toggled

#### Scenario: Inverted behaviour
- **WHEN** the setting is on and DPAD_MIDDLE is pressed briefly on a light tile
- **THEN** the light control screen opens, and holding DPAD_MIDDLE for 800 ms toggles the light instead

#### Scenario: Unavailable row in a group
- **WHEN** DPAD_MIDDLE is pressed short or long on the row of an open group whose entity state is Unavailable
- **THEN** no command is sent, no screen opens and the notification "<name> is unavailable" is shown
