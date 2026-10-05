## ADDED Requirements

### Requirement: Power modes
The UI SHALL track the remote's power mode as one of Normal, Idle, Low_power and Suspend. The mode SHALL be taken from the core's power mode change events and from the response to a power mode request that is sent on every connection to the core. Every mode change SHALL be published to the UI with the previous and the new mode.

#### Scenario: Core reports a change
- **WHEN** the core sends a power mode change event with mode `SUSPEND`
- **THEN** the UI's power mode becomes Suspend and the change (previous mode, Suspend) is published

#### Scenario: Reconnect
- **WHEN** the connection to the core is (re-)established
- **THEN** the current power mode, battery level, power supply and charging status are requested and applied

### Requirement: Window and touch handling per power mode
The UI SHALL hide its window when the power mode goes from Idle to Low_power and show it again when the mode becomes Normal. On a device (not on desktop) the UI SHALL drop all touch and mouse input while the mode is Low_power and accept it again when the mode is Normal.

#### Scenario: Entering low power
- **WHEN** the mode changes from Idle to Low_power on a device
- **THEN** the window is hidden and touches are ignored

#### Scenario: Waking up
- **WHEN** the mode becomes Normal
- **THEN** the window is shown and touch input works again

### Requirement: Command retry window after wakeup
The UI SHALL retry entity commands issued around a wakeup for the "Retry commands after wakeup" window configured under Power Saving (0..10 s, default 2 s, "Disabled" at 0, stored on the remote). A command is eligible when the window is larger than 0, the remote has gone to Suspend and has not yet been reported awake, or the window is currently open, and the command is not a key repeat. An eligible command that fails SHALL be sent again every 500 ms until its deadline; the deadline is provisionally now + window and is extended to wakeup + window once the core reports Normal after a Suspend. After the deadline the normal failure handling applies. A command still in flight after 200 ms SHALL show the loading indicator.

#### Scenario: Button press that wakes the remote
- **WHEN** a button pressed while the remote sleeps sends a command that fails because the integrations are still coming back
- **THEN** the command is resent every 500 ms and the failure is reported only if it has not gone through by the end of the window measured from the wakeup

#### Scenario: Window disabled
- **WHEN** the retry window is set to 0
- **THEN** commands are never retried after a wakeup and fail immediately

#### Scenario: Held button
- **WHEN** a key repeat of a held button fails during the window
- **THEN** it is dropped without retry and without a prompt

#### Scenario: Activity start during wakeup
- **WHEN** an activity is started while the remote is still waking up
- **THEN** its readiness check is repeated every 1 s for the same window while the core is connected, and the "devices not ready" question is asked only once the window is spent

### Requirement: Integration setup keep-alive across standby
While an integration setup session with a keep-alive interval is active the UI SHALL renew it periodically, and SHALL renew it immediately when the power mode becomes Normal and when the connection to the core is re-established. While the remote runs on battery the setup dialog SHALL show the remaining time before the setup ends and warn when a low battery is the reason it is shortened.

#### Scenario: Wake during setup
- **WHEN** the remote wakes from standby while a setup session is active
- **THEN** a keep-alive is sent right away

### Requirement: Battery status
The UI SHALL track the battery level (0..100), whether the remote is charging (core status `CHARGING`) and whether a power supply is connected, from the core's battery status events and from the power mode response on connect. The battery SHALL be reported as low when the level is at or below 10 %.

#### Scenario: Level drops to 10
- **WHEN** the core reports capacity 10
- **THEN** the battery is marked low

#### Scenario: Level rises to 11
- **WHEN** the core reports capacity 11
- **THEN** the low mark is cleared

### Requirement: Battery indicator
The status bar SHALL show a battery icon whose fill is proportional to the level (plus 2 px below 10 %), red when the battery is low and off-white otherwise. While charging the icon SHALL be replaced by a bolt and the level in percent. The percentage SHALL also be shown when "Show battery percentage" is on; holding the icon for 500 ms SHALL toggle that setting. With "Show battery everywhere" on, the same indicator SHALL appear in the title bar of entity and activity screens. Both settings live under "User interface" and are stored on the remote (default off).

#### Scenario: Low battery
- **WHEN** the level is 8 % and not charging
- **THEN** a red, nearly empty battery icon is shown

#### Scenario: Charging
- **WHEN** the remote is charging at 55 %
- **THEN** a bolt and "55" are shown instead of the battery icon

#### Scenario: Toggle percentage
- **WHEN** the user holds the battery icon for 500 ms
- **THEN** the percentage display is toggled and persisted

### Requirement: Battery warnings from the core
When the core sends the warning event `LOW_BATTERY` the UI SHALL show an actionable warning notification titled "Low battery" with "%1% battery remaining. Please charge the remote soon." (current level). For `BATTERY_UNDERVOLT` the text SHALL be "Low battery voltage detected. Charge the battery to 100% before using the remote again.". The warning's shutdown flag and message SHALL be ignored.

#### Scenario: Low battery event
- **WHEN** the core sends LOW_BATTERY at level 5
- **THEN** a "Low battery" notification with "5% battery remaining. Please charge the remote soon." is shown

### Requirement: Charging screen
The UI SHALL show a full-screen charging screen when a power supply becomes connected (playing the BatteryCharge sound) and when the power mode becomes Normal from any mode other than Idle while charging with a power supply connected. The screen SHALL show an analogue clock (12 dots, hour, minute and second hands) and a bolt with "<level>% - Charging" or "<level>% " when supplied without charging. It SHALL close when the power supply is removed, when the screen is tapped, and on the press of BACK, HOME, VOICE, VOLUME_UP, VOLUME_DOWN, GREEN, YELLOW, RED, BLUE, DPAD_UP, DPAD_DOWN, DPAD_LEFT, DPAD_RIGHT, DPAD_MIDDLE, CHANNEL_UP, CHANNEL_DOWN, MUTE, PREV, PLAY, NEXT, POWER, STOP, RECORD or MENU. Open and close SHALL fade over 300 ms.

#### Scenario: Docked
- **WHEN** the remote is put on the charger
- **THEN** the BatteryCharge sound plays and the charging screen with the clock appears

#### Scenario: Wake on charger
- **WHEN** the remote wakes from Suspend while charging on the charger
- **THEN** the charging screen is shown

#### Scenario: Reboot on charger
- **WHEN** the UI starts on a charger and the first power mode reported is Normal with previous mode Idle
- **THEN** the charging screen is not shown

#### Scenario: Key press on the charging screen
- **WHEN** the user presses DPAD_MIDDLE while the charging screen is open
- **THEN** the charging screen closes
- **AND** the key also reaches the keyboard focus chain of the screen underneath

### Requirement: Power off and reboot
Holding POWER for 3 s SHALL open the power-off screen unless a software update is in progress. The screen SHALL offer "Power off" and "Reboot" as press-and-hold buttons (1 s hold, "Press and hold" hint, progress fill) and "Cancel". With the keypad, DPAD_UP/DPAD_DOWN SHALL move the selection between Power off, Reboot and Cancel, and DPAD_MIDDLE SHALL trigger the selected entry immediately; BACK and HOME SHALL close the screen. Triggering SHALL play a Click haptic and the ClickLow sound and send the system command `POWER_OFF` or `REBOOT`; a rejected command SHALL show "Error on power off: <message>" or "Error on reboot: <message>". Blocking the input (loading screen) SHALL reset the power hold.

#### Scenario: Long press power
- **WHEN** POWER is held for 3 s
- **THEN** the power-off screen opens

#### Scenario: Hold power off
- **WHEN** the user holds "Power off" for 1 s
- **THEN** `POWER_OFF` is sent to the core

#### Scenario: Keypad confirm
- **WHEN** "Reboot" is selected with the d-pad and DPAD_MIDDLE is pressed
- **THEN** `REBOOT` is sent without a hold delay

#### Scenario: Update running
- **WHEN** a software update is in progress
- **THEN** holding POWER does nothing

### Requirement: Power saving settings
The "Power Saving" page SHALL show, in this order: "Keep WiFi connected in standby" (Remote Two always, otherwise only when `UC_WOWLAN=true`), "Retry commands after wakeup" (slider 0..10 s, "Disabled" / "10 seconds", text "Retry commands within %1 second(s) after wakeup."), "Wakeup sensitivity" (slider 0..3 = off, low, medium, high, "Off" / "Sensitivity", text "Amount of movement needed to wake up the remote."), "Display off timeout" (10..60 s) and "Sleep timeout" (10..300 s, shown as time, "10 seconds" / "5 minutes"). Wake-up sensitivity, display off and sleep timeout SHALL be sent together as one power-saving configuration and adopted only after the core confirmed them; failures SHALL show "Error setting wakeup sensitivity: <message>", "Error setting display sleep timeout: <message>" or "Error setting sleep timeout: <message>".

#### Scenario: Sleep timeout changed
- **WHEN** the user sets the sleep timeout to 120 s
- **THEN** the configuration is sent with standby_sec 120 and the current display_off_sec and wakeup_sensitivity
- **AND** the value shows as 2:00 once confirmed

#### Scenario: Sensitivity off
- **WHEN** the user sets wakeup sensitivity to 0
- **THEN** wakeup_sensitivity 0 (off) is sent

#### Scenario: Core reports configuration
- **WHEN** the core's configuration contains power-saving values
- **THEN** the sliders show those values
