## MODIFIED Requirements

### Requirement: Haptic feedback
The UI SHALL provide four haptic effects: Click, Buzz, Error and Bump. Haptics SHALL be enabled or disabled by the "Haptic feedback" switch in "Sound & Haptic", stored in the core's haptic configuration and adopted only after the core confirmed it ("Error changing haptic settings: <message>" on failure). Until the first configuration of the core arrives haptics SHALL be off. While disabled no effect SHALL be played. On Remote Two the effect number 0..3 SHALL be written to the haptic device; on Remote 3 the effects SHALL map to driver effects 1 (Click, strong click 100 %), 25 (Bump, sharp tick), 14 (Buzz) and 45 (Error). A failed write SHALL only be logged.

#### Scenario: Tap feedback
- **WHEN** the user presses any tappable control (buttons, rows, switches, checkboxes, keypad keys, keyboard keys, page selector, popup menus)
- **THEN** a Click effect is played

#### Scenario: Value feedback
- **WHEN** a value changes while dragging a light brightness, colour, cover, climate or touch slider, or the pull-down menu threshold is crossed
- **THEN** a Bump effect is played (touch slider: at most once per 30 ms)

#### Scenario: Error feedback
- **WHEN** an input field or search field rejects input, or a profile switch fails
- **THEN** an Error effect is played (a profile switch additionally plays Buzz)

#### Scenario: Haptics disabled
- **WHEN** the haptic feedback switch is off
- **THEN** no effect reaches the haptic device

#### Scenario: Before the first configuration
- **WHEN** a control is pressed after the start of the app and before the first configuration of the core arrived
- **THEN** no effect reaches the haptic device, also when the user had haptics enabled; effects play once the configuration says they are enabled

### Requirement: Sound effects
The UI SHALL provide the sound effects Click, ClickLow, Confirm, Error and BatteryCharge, loaded at startup from `click.wav`, `click_lo.wav`, `confirm.wav`, `error.wav` and `zap_future.wav` in the directory `UC_SOUND_EFFECTS_PATH` and played on the default audio output device. Playback SHALL be gated by the "Sound effects" switch and scaled by the "Sound effects volume" slider (0..100, played at volume/100), both stored in the core's sound configuration and adopted only after the core confirmed them. When `UC_SOUND_EFFECTS_PATH` is unset the UI SHALL load no effect and log one informational line that sound effects are disabled; when it names a directory that does not exist the UI SHALL load no effect and log one warning naming the directory. In both cases playing an effect SHALL be a no-op and the rest of the UI SHALL work unchanged. A custom UI build installed by a user runs in its own sandbox, in which the firmware's sound effects (like the licensed icon font) are not available on purpose, because licensed files are not exposed to third-party binaries; a custom build therefore SHALL play no sound effects and is otherwise unaffected.

#### Scenario: Effects in use
- **WHEN** a voice request or a long-running action succeeds
- **THEN** Confirm is played, and Error is played when it fails
- **AND** ClickLow is played when the user triggers power off or reboot
- **AND** BatteryCharge is played when a power supply is connected
- **AND** Click is played when the sound volume slider is changed

#### Scenario: Effects disabled
- **WHEN** the sound effects switch is off
- **THEN** nothing is played

#### Scenario: Missing sound directory
- **WHEN** `UC_SOUND_EFFECTS_PATH` is unset, as on the desktop simulator
- **THEN** exactly one informational line reports that sound effects are disabled, no sound file is opened, and every later request to play an effect does nothing

#### Scenario: Configured directory does not exist
- **WHEN** `UC_SOUND_EFFECTS_PATH` names a directory that does not exist
- **THEN** exactly one warning naming that directory is logged, no sound file is opened, and every later request to play an effect does nothing

#### Scenario: Custom build on the device
- **WHEN** a custom UI build runs on a remote in its sandbox
- **THEN** no sound effect is played, the "Sound effects" settings stay operable, and the rest of the UI works unchanged

### Requirement: Device information
On every connection to the core the UI SHALL request the system information and keep the serial number and hardware revision from the response; the model number SHALL be the selected hardware model, known from the start of the app, before any screen reads it, and it SHALL never change while the app runs. The About page SHALL show "Model number", "Serial number" and "Revision". During onboarding the default remote name SHALL be "Remote 3" on `UCR3` and "Remote Two" otherwise.

#### Scenario: About page
- **WHEN** the user opens About on a Remote 3 with serial 1234 and revision 5.1
- **THEN** the page shows Model number UCR3, Serial number 1234, Revision 5.1

#### Scenario: System information request fails
- **WHEN** the core rejects the system information request
- **THEN** the failure is logged and the serial number and revision stay empty

#### Scenario: Model read before the system information arrives
- **WHEN** a screen that depends on the model (the onboarding remote name, the WiFi band selector, the touch slider overlay) is built before the system information response arrived
- **THEN** it already uses the selected model, e.g. "Remote 3" as the default name on a Remote 3
