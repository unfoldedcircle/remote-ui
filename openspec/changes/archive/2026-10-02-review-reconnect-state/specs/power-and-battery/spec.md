## MODIFIED Requirements

### Requirement: Power modes
The UI SHALL track the remote's power mode as one of Normal, Idle, Low_power and Suspend. The mode SHALL be Normal when the UI starts, until the core reports otherwise. The mode SHALL be taken from the core's power mode change events and from the response to one power mode request that is sent on every connection to the core; the same response SHALL supply the battery level, power supply and charging status (see "Battery status"), so no second request is sent for them. Only a change of the mode SHALL be published to the UI, with the previous and the new mode: a reported mode equal to the current one — in particular the answer to the request after a reconnect, and a first answer of Normal after the start — SHALL NOT be published.

#### Scenario: Core reports a change
- **WHEN** the core sends a power mode change event with mode `SUSPEND`
- **THEN** the UI's power mode becomes Suspend and the change (previous mode, Suspend) is published

#### Scenario: Reconnect
- **WHEN** the connection to the core is (re-)established
- **THEN** one power mode request is sent, and the current power mode, battery level, power supply and charging status are taken from its answer
- **AND** the power mode is published only when it differs from the mode the UI already holds

#### Scenario: Reconnect after a core restart
- **WHEN** the app reconnects after a restart of the core and the core reports Normal while the UI already holds Normal
- **THEN** no power mode change is published, so nothing that reacts to a wake-up runs

#### Scenario: First answer after the start
- **WHEN** the first power mode answer after the app starts reports Normal
- **THEN** no power mode change is published, because the mode already is Normal

### Requirement: Charging screen
The UI SHALL show a full-screen charging screen when a power supply becomes connected (playing the BatteryCharge sound) and when the power mode changes to Normal from any mode other than Idle while charging with a power supply connected. A reported power mode equal to the current one is not a change (see "Power modes"), so a reconnect to the core SHALL NOT open the charging screen. The power supply is taken as not connected when the UI starts, so a first battery status that reports a connected power supply counts as connecting it. The screen SHALL show an analogue clock (12 dots, hour, minute and second hands) and a bolt with "<level>% - Charging" or "<level>% " when supplied without charging. It SHALL close when the power supply is removed, when the screen is tapped, and on the press of BACK, HOME, VOICE, VOLUME_UP, VOLUME_DOWN, GREEN, YELLOW, RED, BLUE, DPAD_UP, DPAD_DOWN, DPAD_LEFT, DPAD_RIGHT, DPAD_MIDDLE, CHANNEL_UP, CHANNEL_DOWN, MUTE, PREV, PLAY, NEXT, POWER, STOP, RECORD or MENU. Open and close SHALL fade over 300 ms.

#### Scenario: Docked
- **WHEN** the remote is put on the charger
- **THEN** the BatteryCharge sound plays and the charging screen with the clock appears

#### Scenario: Wake on charger
- **WHEN** the remote wakes from Suspend while charging on the charger
- **THEN** the charging screen is shown

#### Scenario: Reboot on charger
- **WHEN** the UI starts on a charger
- **THEN** the first power mode answer (Normal) publishes no power mode change
- **AND** the charging screen appears and the BatteryCharge sound plays once, because the first battery status reports the power supply as connected

#### Scenario: Reconnect on the charger
- **WHEN** the app reconnects to the core, for example after a restart of the core, while the remote lies on the charger
- **THEN** the charging screen does not open and no sound plays, because neither the power mode nor the power supply changed

#### Scenario: Key press on the charging screen
- **WHEN** the user presses DPAD_MIDDLE while the charging screen is open
- **THEN** the charging screen closes
- **AND** the key also reaches the keyboard focus chain of the screen underneath
