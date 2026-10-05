# hardware-platform Specification

## Purpose

The hardware abstraction of the UI: model selection, per-model drivers, screen geometry and rotation, brightness, haptic feedback, sound effects and device information.

## Requirements

### Requirement: Hardware model selection
The UI SHALL select the hardware model from the environment variable `UC_MODEL`, compared case-insensitively against `DEV`, `UCR2` and `UCR3`. An unset or unrecognised value SHALL select `DEV`. The selected model SHALL be reported as the model number everywhere the UI shows or checks it; the model name reported by the core is ignored. The model of the first YIO remote is not supported and SHALL NOT exist as a value.

#### Scenario: Valid model
- **WHEN** the app starts with `UC_MODEL=ucr3`
- **THEN** the UI runs as Remote 3 and reports model number `UCR3`

#### Scenario: Invalid or missing model
- **WHEN** the app starts with `UC_MODEL` unset or set to an unknown value
- **THEN** the UI runs as the desktop simulator (`DEV`)

#### Scenario: Legacy YIO1 model
- **WHEN** the app starts with `UC_MODEL=YIO1`
- **THEN** the value is unrecognised and the UI runs as the desktop simulator (`DEV`), which is what `YIO1` behaved like

### Requirement: Hardware features per model
The UI SHALL enable hardware drivers according to the model: Remote Two gets a haptic driver on the device given by `UC_HAPTIC_DEV_PATH`; Remote 3 gets a haptic driver on `UC_HAPTIC_DEV_PATH` and a touch slider driver on `UC_TOUCHSLIDER_DEV_PATH`; all other models get a no-op haptic driver and a touch slider that never reports input.

#### Scenario: Remote Two
- **WHEN** the model is `UCR2`
- **THEN** haptic effects are written to the haptic device and the touch slider never produces events

#### Scenario: Remote 3
- **WHEN** the model is `UCR3`
- **THEN** haptic effects are written to the haptic device and touch slider events are read from the slider input device

#### Scenario: Desktop
- **WHEN** the model is `DEV`
- **THEN** haptic effects and touch slider input are silently unavailable

### Requirement: Screen geometry
On a device the UI SHALL use the full geometry of the primary screen. On desktop (`DEV`) the UI SHALL use `UC_DISPLAY_WIDTH` x `UC_DISPLAY_HEIGHT` (default 480 x 850) logical pixels with `UC_DISPLAY_SCALE` as the global scale factor and high-DPI scaling enabled, the default scale being 1 on Linux and Windows and 0.5 on macOS; the window size and placement are specified in `desktop-simulator`. The desktop window title SHALL be "Remote Two simulator".

#### Scenario: Desktop defaults
- **WHEN** the app starts as `DEV` without display variables on Linux or Windows
- **THEN** the UI is laid out for 480 x 850 logical pixels and drawn at that size (see `desktop-simulator`)

#### Scenario: Desktop defaults on macOS
- **WHEN** the app starts as `DEV` without display variables on macOS
- **THEN** the UI is laid out for 480 x 850 logical pixels and drawn at half size, which is its intended physical size on a 2x Retina display

#### Scenario: Device
- **WHEN** the app starts on a device
- **THEN** the UI fills the screen at the panel's native resolution

### Requirement: Remote Two screen rotation
On Remote Two the display panel is landscape and the UI SHALL be rendered in portrait by swapping width and height, rotating the root item by -90 degrees and reporting inverted-landscape content orientation to the window. The on-screen keyboard SHALL be rotated with the UI and slide in from the panel's right edge (the bottom of the rotated UI), and edge drawers SHALL open from the panel's left edge instead of its top edge. Remote 3 and desktop SHALL not rotate.

#### Scenario: Rotated UI on Remote Two
- **WHEN** the model is `UCR2`
- **THEN** a portrait UI of W x H is displayed rotated on the H x W panel
- **AND** the keyboard is parked 20 px beyond the panel edge and shown at (panel height - keyboard height + 14) px

#### Scenario: No rotation on Remote 3
- **WHEN** the model is `UCR3` or `DEV`
- **THEN** the UI is displayed unrotated and the keyboard hides below the bottom edge

### Requirement: Rendering and text setup
The UI SHALL render with OpenGL ES, use native text rendering with distance-field text disabled, load the icon font from the file named in `UC_ICON_FONT_PATH` when the firmware provides one and from the embedded resources otherwise, and use "Poppins" as the primary font and "Space Mono" as the secondary font. The corner radii used by the UI SHALL be 8 px (small) and 22 px (large). Text input SHALL use the Qt virtual keyboard with the embedded layouts and the "remotestyle" style.

#### Scenario: Startup
- **WHEN** the app starts
- **THEN** icons are loaded from the icon font — the firmware's file or the embedded one — and text is rendered natively
- **AND** every text field opens the embedded virtual keyboard

### Requirement: Desktop button simulator
On desktop (`DEV`) the UI SHALL open a second window emulating the physical buttons, placed below the main window (or to its right when the screen is too small), which never takes keyboard focus. The window SHALL not be created on models that show regulatory information.

#### Scenario: Desktop
- **WHEN** the model is `DEV`
- **THEN** a "Button simulator" window is shown next to the main window and the main window keeps the keyboard focus

#### Scenario: Device
- **WHEN** the model is `UCR2` or `UCR3`
- **THEN** no button simulator window exists

### Requirement: Regulatory information flag
The UI SHALL expose a regulatory-information flag that is true on Remote Two and Remote 3 and false on the desktop. The flag SHALL only decide whether the desktop button simulator window is created; the About section lists "Regulatory" on every model.

#### Scenario: Device
- **WHEN** the model is `UCR2` or `UCR3`
- **THEN** the flag is true and no button simulator window is created

#### Scenario: Desktop
- **WHEN** the model is `DEV`
- **THEN** the flag is false and the button simulator window is shown; the About section still lists "Regulatory"

### Requirement: Key navigation availability per model
The UI SHALL report keypad navigation as enabled for the models `UCR2`, `UCR3` and `DEV`, i.e. for every model. Key navigation SHALL be reported as active only while it is enabled and the keypad is currently in use (the last input came from a physical key rather than from touch).

#### Scenario: Keypad in use
- **WHEN** a physical key is pressed on any supported model
- **THEN** key navigation becomes active and keypad selection highlights are rendered

#### Scenario: Touch resumes
- **WHEN** the screen is touched
- **THEN** key navigation becomes inactive and the highlights disappear

### Requirement: Display brightness settings
The "Display & Brightness" settings SHALL offer an "Auto brightness" switch ("Automatically adjust the display brightness based on ambient lighting conditions."), a "Display brightness" slider from 5 to 100 in steps of 1, a "Button backlight" switch ("When on, button backlight will automatically turn on in a dark room.") and a "Button backlight brightness" slider from 0 to 100. Display values SHALL be sent to the core as one display configuration (brightness + auto flag), button values as one button configuration. The UI SHALL adopt a value only after the core confirmed it and SHALL show an error notification "Error setting display config: <message>" or "Error setting button backlight: <message>" otherwise. Values in the configuration received from the core SHALL overwrite the local values.

#### Scenario: Brightness slider moved
- **WHEN** the user moves the display brightness slider to 40
- **THEN** a display configuration with brightness 40 and the current auto-brightness flag is sent
- **AND** the shown value changes once the core acknowledges

#### Scenario: Core rejects
- **WHEN** the core answers with an error
- **THEN** an error notification with the core's message is shown and the previous value stays

#### Scenario: Minimum brightness
- **WHEN** the user drags the display brightness slider to its lowest position
- **THEN** the value is 5, never lower

### Requirement: Software dimming on Remote Two
On Remote Two the UI SHALL additionally dim its own output with a black overlay whose opacity is (100 - display brightness) / 100, covering the UI, the popups and the on-screen keyboard, animated over 300 ms. The animation SHALL finish also while the window is hidden with the display off, so that a brightness change with the display off leaves no animation running. The only such change is the configured brightness arriving when the UI starts with the display off: a brightness change through the Core-API, such as one in the web-configurator, makes the core turn the display on, on every model. Remote 3 and desktop SHALL not apply software dimming.

#### Scenario: Brightness 30 on Remote Two
- **WHEN** the display brightness is 30 on `UCR2`
- **THEN** a black layer with opacity 0.7 is drawn over the entire UI including the keyboard

#### Scenario: Remote 3
- **WHEN** the display brightness changes on `UCR3`
- **THEN** the rendered UI is unchanged; only the hardware backlight is affected

#### Scenario: Brightness change with the display off
- **WHEN** the UI starts on `UCR2` while the display is off and the configured brightness arrives
- **THEN** the overlay reaches its opacity while the window stays hidden, and no animation keeps the main thread busy

### Requirement: Display off timeout
The "Power Saving" settings SHALL offer a "Display off timeout" slider from 10 to 60 seconds, shown as "<n>s", sent to the core together with the standby timeout and the wake-up sensitivity as one power-saving configuration and adopted only after the core confirmed it ("Error setting display sleep timeout: <message>" on failure).

#### Scenario: Timeout changed
- **WHEN** the user sets the display off timeout to 30 s
- **THEN** the power-saving configuration is sent with display_off_sec 30 and the current standby_sec and wakeup_sensitivity

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

### Requirement: Case-open warning
When the core reports the warning event `OPEN_CASE` the UI SHALL hide the on-screen keyboard and show a non-dismissable full-screen warning with a red ban icon, the text "Do not operate the device disassembled." and "The remote will turn off\nin %1 seconds." counting down once per second from 2 and stopping at 0.

#### Scenario: Case opened
- **WHEN** the core sends a warning with event OPEN_CASE
- **THEN** the warning screen is shown and stays until the core powers the remote off

### Requirement: Process lifecycle and runtime environment
The UI SHALL quit cleanly on SIGINT, SIGQUIT and SIGTERM so the system service manager does not treat a stop as a crash, and SHALL exit with status -1 when the main UI cannot be loaded. The core socket URL SHALL come from `UC_SOCKET_URL` (default `ws://127.0.0.1:8080/ws`); resources and legal texts SHALL be read from `UC_RESOURCE_PATH` and `UC_LEGAL_PATH`; onboarding mode SHALL be active whenever the file named by `UC_ONBOARDING_PATH` exists.

#### Scenario: Service stop
- **WHEN** the service manager sends SIGTERM
- **THEN** the app exits normally without triggering the recovery handler

#### Scenario: Onboarding marker present
- **WHEN** the file at `UC_ONBOARDING_PATH` exists at startup
- **THEN** the UI starts in onboarding mode
