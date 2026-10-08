## RENAMED Requirements

- FROM: `### Requirement: Press and hold in the button simulator is limited`
- TO: `### Requirement: Press and hold in the button simulator auto-repeats`

## MODIFIED Requirements

### Requirement: Emulated buttons and their key events
The button simulator SHALL offer exactly 21 click areas laid over the keypad picture, in a 480 px wide layout: top row BACK (96 x 90), HOME (288 x 90), VOICE (96 x 90); a middle block of 280 px with VOLUME_UP and VOLUME_DOWN stacked on the left (90 x 140 each), a 3 x 3 grid of 96 x 93 areas GREEN, DPAD_UP, YELLOW / DPAD_LEFT, DPAD_MIDDLE, DPAD_RIGHT / RED, DPAD_DOWN, BLUE, and CHANNEL_UP and CHANNEL_DOWN stacked on the right; bottom row MUTE, PREV, PLAY, NEXT, POWER (96 x 90 each). Pressing an area SHALL send the key press event of that physical button to the main window and releasing the mouse button SHALL send the matching key release, so the event takes the same paths as a hardware key (see `key-navigation`). When the window system cancels the press (the area loses the mouse grab), the simulator SHALL send the release at that moment. A pressed area SHALL be tinted off-white, fading over 300 ms. The Remote 3 buttons STOP, RECORD and MENU SHALL NOT be emulated. The areas SHALL line up with the picture only while the window is 480 logical pixels wide, i.e. at scale 1, which is the default on Linux and Windows.

#### Scenario: Clicking a d-pad button
- **WHEN** the user clicks DPAD_DOWN in the button simulator
- **THEN** one DPAD_DOWN press and one DPAD_DOWN release reach the main window, exactly as from the device keypad

#### Scenario: Release outside the area
- **WHEN** the user presses HOME, drags the mouse off the area and releases it
- **THEN** the HOME release is still sent

#### Scenario: Cancelled press
- **WHEN** the window system cancels the press of a held simulator button
- **THEN** the release of that button is sent and no further press follows

#### Scenario: Remote 3 only button
- **WHEN** a developer needs STOP or RECORD on desktop
- **THEN** no click area exists for them

#### Scenario: Default scale on a regular display
- **WHEN** the simulator runs with the default scale on a Linux or Windows 1x display
- **THEN** the click areas match the keypad picture

#### Scenario: Scale other than 1
- **WHEN** the simulator runs at scale 0.5 on a 1x display
- **THEN** the click areas are laid out for a 960 px wide window and no longer match the keypad picture

### Requirement: Press and hold in the button simulator auto-repeats
A held simulator button SHALL send the key events of a held device key: one press when the mouse button goes down; while it stays down, a press flagged as auto-repeat 600 ms after the first press and then every 150 ms, with no release in between; and one release, not flagged as auto-repeat, when the mouse button goes up. 600 ms and 150 ms are the auto-repeat delay and rate the firmware sets for the device keypad (see `platform-constraints`). Every emulated button SHALL repeat. A release before 600 ms SHALL send no auto-repeat press. The repeat presses therefore take the device paths of `key-navigation` ("Short press, repeat, long press and release semantics"): `pressed_repeat` (or `pressed`) runs for each of them, while a key with a `long_press` handler ignores them and still runs `long_press` after 800 ms.

#### Scenario: Short click
- **WHEN** the user clicks DPAD_DOWN and releases it after 300 ms
- **THEN** one press and one release are sent and no auto-repeat press

#### Scenario: Holding a d-pad button
- **WHEN** the user holds the DPAD_DOWN area for 2 s on a list that supports repeat
- **THEN** the selection keeps moving, as on a device: one press, auto-repeat presses at 600 ms, 750 ms, 900 ms and so on, and one release when the mouse button goes up

#### Scenario: Release at the end of a hold
- **WHEN** the user releases a held DPAD_DOWN area after auto-repeat presses were sent
- **THEN** the release is delivered at once, not deferred, and any `released` handler runs

#### Scenario: Long press
- **WHEN** the user holds the HOME area for 1 s
- **THEN** the HOME long-press action runs once and the short-press action does not run

#### Scenario: Power menu
- **WHEN** the user holds the POWER area for 3 s while no software update is running
- **THEN** the power off menu opens

### Requirement: Desktop verification limits
A desktop run SHALL NOT be treated as verification of behaviour that needs device hardware or core functions the Remote-Core Simulator does not provide: haptics, the touch slider, battery and charging, power modes and suspend/resume, WiFi and Bluetooth hardware, the physical keypad (only the Remote Two keys are emulated), mDNS discovery of docks and integrations, and installing custom integrations. Everything else runs the same code as on the device and behaves the same against the Remote-Core Simulator.

#### Scenario: Keypad verification
- **WHEN** a change affects d-pad navigation
- **THEN** it is walked with the keypad on a device, or with the computer keyboard or a held simulator button in `DEV`, not only by clicking the button simulator
