## ADDED Requirements

### Requirement: Simulator main window
On desktop (DEV) the UI SHALL be laid out at `UC_DISPLAY_WIDTH` x `UC_DISPLAY_HEIGHT` logical pixels (default 480 x 850; unset, 0 or non-numeric selects the default), unrotated, with high-DPI scaling enabled and `UC_DISPLAY_SCALE` (default 0.5; unset, 0 or non-numeric selects the default) applied as the global Qt scale factor. The main window SHALL have a fixed size of the UI size divided by the scale factor and the title "Remote Two simulator", with the UI centred in it on a black background. The UI therefore fills the window only at scale 1; at any other scale it occupies that fraction of the window's width and height. Model selection itself is specified in `hardware-platform`.

#### Scenario: Scale 1 on a regular display
- **WHEN** the simulator starts with `UC_DISPLAY_SCALE=1` and no size variables on a 1x display
- **THEN** a non-resizable 480 x 850 window opens and the UI fills it exactly

#### Scenario: Default scale on a regular display
- **WHEN** the simulator starts without `UC_DISPLAY_SCALE` on a 1x display
- **THEN** the window is 480 x 850 screen pixels and the UI is drawn at 240 x 425 in its centre
- **AND** the on-screen keyboard is visible below the UI in its parked position

#### Scenario: Custom size
- **WHEN** the simulator starts with `UC_DISPLAY_WIDTH=400`, `UC_DISPLAY_HEIGHT=700` and `UC_DISPLAY_SCALE=1`
- **THEN** the UI is laid out for 400 x 700 in a fixed 400 x 700 window

### Requirement: Button simulator window
On every model that does not show regulatory information (desktop DEV, and YIO1) the UI SHALL open a second window titled "Button simulator" with a black background, fixed at the main window's width and 0.95 times that width in height, showing a picture of the Remote Two keypad. The window SHALL be placed 60 px below the main window, left-aligned with it, or, when the main window's height plus the simulator's height exceed the available desktop height, directly to the right of the main window aligned with its top edge. Its content SHALL only be loaded while the window is visible. On Remote Two and Remote 3 the window SHALL stay hidden and empty.

#### Scenario: Tall desktop
- **WHEN** the simulator starts at scale 1 on a screen with at least 1306 px (850 + 456) of available height
- **THEN** a 480 x 456 "Button simulator" window appears 60 px below the main window

#### Scenario: Short desktop
- **WHEN** the combined window heights exceed the available desktop height
- **THEN** the button simulator window appears to the right of the main window, top edges aligned

#### Scenario: Device model
- **WHEN** `UC_MODEL` is `UCR2` or `UCR3`
- **THEN** only the main window is shown

### Requirement: Emulated buttons and their key events
The button simulator SHALL offer exactly 21 click areas laid over the keypad picture, in a 480 px wide layout: top row BACK (96 x 90), HOME (288 x 90), VOICE (96 x 90); a middle block of 280 px with VOLUME_UP and VOLUME_DOWN stacked on the left (90 x 140 each), a 3 x 3 grid of 96 x 93 areas GREEN, DPAD_UP, YELLOW / DPAD_LEFT, DPAD_MIDDLE, DPAD_RIGHT / RED, DPAD_DOWN, BLUE, and CHANNEL_UP and CHANNEL_DOWN stacked on the right; bottom row MUTE, PREV, PLAY, NEXT, POWER (96 x 90 each). Pressing an area SHALL send the key press event of that physical button to the main window and releasing the mouse button SHALL send the matching key release, so the event takes the same paths as a hardware key (see `key-navigation`). A pressed area SHALL be tinted off-white, fading over 300 ms. The Remote 3 buttons STOP, RECORD and MENU SHALL NOT be emulated. The areas SHALL line up with the picture only while the window is 480 logical pixels wide, i.e. at scale 1.

#### Scenario: Clicking a d-pad button
- **WHEN** the user clicks DPAD_DOWN in the button simulator
- **THEN** one DPAD_DOWN press and one DPAD_DOWN release reach the main window, exactly as from the device keypad

#### Scenario: Release outside the area
- **WHEN** the user presses HOME, drags the mouse off the area and releases it
- **THEN** the HOME release is still sent

#### Scenario: Remote 3 only button
- **WHEN** a developer needs STOP or RECORD on desktop
- **THEN** no click area exists for them

#### Scenario: Default scale on a regular display
- **WHEN** the simulator runs at scale 0.5 on a 1x display
- **THEN** the click areas are laid out for a 960 px wide window and no longer match the keypad picture

### Requirement: Press, hold and long press from the simulator
A simulator button SHALL produce exactly one key press when the mouse button goes down and one key release when it goes up, with no auto-repeat in between. Holding a simulator button SHALL therefore trigger time-based long-press behaviour (800 ms long-press handlers, the 3 s POWER hold) but never repeat handlers.

#### Scenario: Long press
- **WHEN** the user holds the HOME area for 1 s
- **THEN** the HOME long-press action runs once and the short-press action does not run

#### Scenario: Holding a d-pad button
- **WHEN** the user holds the DPAD_DOWN area for 2 s on a list
- **THEN** the selection moves by one entry only

#### Scenario: Power menu
- **WHEN** the user holds the POWER area for 3 s while no software update is running
- **THEN** the power off menu opens

### Requirement: Desktop keyboard as keypad
When the UI runs on a desktop (any model), key events from the computer keyboard SHALL be handled like physical buttons whenever their key code belongs to the button map: the arrow keys act as DPAD_UP / DPAD_DOWN / DPAD_LEFT / DPAD_RIGHT, Return as DPAD_MIDDLE, Home as HOME, F3 as VOICE and F4 as MENU. Escape, Backspace and the numeric keypad Enter SHALL NOT act as any button. A held keyboard key auto-repeats like a held hardware key. Any other key only reaches the focused control (e.g. typing into a text field).

#### Scenario: Arrow keys
- **WHEN** the user presses the Down arrow key on the desktop keyboard
- **THEN** the UI reacts as to DPAD_DOWN and the keypad selection highlights become visible

#### Scenario: Escape
- **WHEN** the user presses Escape on a settings page
- **THEN** the page is not left; BACK has to be clicked in the button simulator

#### Scenario: Held arrow key
- **WHEN** the user holds the Down arrow key on a list that supports repeat
- **THEN** the selection keeps moving until the key is released

### Requirement: Keyboard focus with the second window
The button simulator window SHALL be created as a window that never accepts focus, so that clicking it leaves the main window active and the main window keeps its active focus item. Focus-chain navigation (keyboard focus highlight, up/down focus hand-over, Return on a focused control) SHALL therefore work on desktop (DEV) as on the device. The focus chain SHALL only react while the main window is the active desktop window; the button ownership path reacts regardless. A single-window run (`UC_MODEL=UCR2` or `UCR3`) SHALL remain available to check focus navigation without any second window.

#### Scenario: Clicking the simulator
- **WHEN** the user clicks DPAD_DOWN in the button simulator on a focus-chain settings page
- **THEN** the main window stays active and the focus highlight moves to the next control

#### Scenario: Main window inactive
- **WHEN** another desktop application is active and a simulator button is clicked
- **THEN** screens driven by button ownership still react, but focus-chain screens show no highlight and do not move

### Requirement: Hardware behaviour on desktop
On desktop (DEV) the UI SHALL run without device hardware: haptic effects SHALL be silently dropped; the touch slider SHALL never produce input, although slider overlays are armed as on Remote 3; touch and mouse input SHALL never be blocked by the low-power mode; no software dimming overlay SHALL be drawn; the UI SHALL not be rotated. WiFi, Bluetooth, power, battery and system information SHALL come only from the core. Where screens differ per model, desktop SHALL follow Remote 3: the "WiFi band" selector and the "WPA3 Personal" security option are offered, media artwork on the media player screens is 60 px shorter than its width, and the wake-on-WLAN rows are shown only with `UC_WOWLAN=true`. The model number SHALL read "DEV" once the core answered the system information request, the onboarding remote name default SHALL be "Remote Two", and the About page SHALL still list the Regulatory entry.

#### Scenario: Haptic feedback on desktop
- **WHEN** the user taps a button on desktop with haptic feedback enabled
- **THEN** nothing is written to any device and nothing is logged

#### Scenario: Low power on desktop
- **WHEN** the core reports the LOW_POWER mode on desktop
- **THEN** mouse clicks on the UI are still accepted

#### Scenario: WiFi settings on desktop
- **WHEN** the user opens the WiFi settings on desktop
- **THEN** the "WiFi band" row is shown, as on Remote 3

### Requirement: Device models on a desktop
When `UC_MODEL` is `UCR2` or `UCR3` on a desktop the UI SHALL open only the main window, sized and fixed to the full geometry of the primary screen, with no desktop scale factor applied and `UC_DISPLAY_WIDTH`, `UC_DISPLAY_HEIGHT` and `UC_DISPLAY_SCALE` ignored. With `UCR2` the width and height SHALL be swapped and the UI rotated by -90 degrees as on the device. The device drivers SHALL be active: without a haptic device at `UC_HAPTIC_DEV_PATH` every haptic effect logs "Failed to write to haptic device"; with `UCR3` and no device at `UC_TOUCHSLIDER_DEV_PATH` the touch slider driver retries opening it every 1 s and logs a warning on every attempt. Input then comes only from mouse, touch and the computer keyboard.

#### Scenario: Remote 3 on a desktop
- **WHEN** the app starts with `UC_MODEL=UCR3` on a 1920 x 1080 screen
- **THEN** one fixed 1920 x 1080 window opens with an unrotated UI and no button simulator

#### Scenario: Remote Two on a desktop
- **WHEN** the app starts with `UC_MODEL=UCR2` on a 1920 x 1080 screen
- **THEN** the UI is laid out for 1080 x 1920 and shown rotated by -90 degrees

#### Scenario: Missing touch slider device
- **WHEN** the app runs with `UC_MODEL=UCR3` and `UC_TOUCHSLIDER_DEV_PATH` does not exist
- **THEN** a "Touch slider device does not exist" warning is logged once per second

### Requirement: Remote-Core Simulator dependency
On desktop the UI SHALL obtain all configuration, entities and state from a remote-core at `UC_SOCKET_URL` (default `ws://127.0.0.1:8080/ws`), normally the Remote-Core Simulator started with docker-compose, authenticating with the token file named by `UC_TOKEN_PATH`, which for the simulator is `$CORE_SIMULATOR_PATH/docker/ui-env/ws-token`. Connection and authentication behaviour are specified in `core-connection`; the startup screen in `app-startup`.

#### Scenario: Simulator not running
- **WHEN** the app starts on desktop while no core listens on `UC_SOCKET_URL`
- **THEN** the startup screen stays and a connection problem is reported

#### Scenario: Token path not set
- **WHEN** the app starts on desktop without `UC_TOKEN_PATH`
- **THEN** the UI never becomes authenticated and the startup screen stays

### Requirement: Desktop verification limits
A desktop run SHALL NOT be treated as verification of behaviour that needs device hardware or core functions the Remote-Core Simulator does not provide: haptics, the touch slider, battery and charging, power modes and suspend/resume, WiFi and Bluetooth hardware, the physical keypad (only the Remote Two subset is emulated), admin PIN validation, mDNS discovery of docks and integrations, and installing custom integrations. The Remote-Core Simulator does not answer the admin PIN request, so the onboarding PIN step cannot be completed on desktop.

#### Scenario: Onboarding PIN on desktop
- **WHEN** the user enters and confirms an admin PIN during onboarding on desktop
- **THEN** the step does not advance, because the core simulator does not confirm the PIN
- **AND** after the request timeout (10 s by default) the notification "Error white setting admin pin: Request timed out" is shown and the first PIN keypad returns

#### Scenario: Keypad verification
- **WHEN** a change affects d-pad navigation
- **THEN** it is walked with the keypad in a single-window run or on a device, not only by clicking the button simulator

### Requirement: Developer environment scripts
The environment scripts SHALL export each variable only when it is not already set, so a value set before sourcing wins. `scripts/env/linux.sh` SHALL set `QT_VERSION` 5.15.2, `QTDIR` `$HOME/Qt/<QT_VERSION>/gcc_64`, prepend `$QTDIR/bin` to `PATH` and `$QTDIR/lib` to `LD_LIBRARY_PATH`, set `QT_PLUGIN_PATH` to `$QTDIR/plugins`, `QT_QPA_PLATFORM` xcb, `UC_MODEL` DEV, `UC_DISPLAY_WIDTH` 480, `UC_DISPLAY_HEIGHT` 850, `UC_DISPLAY_SCALE` 1 and `UC_TOKEN_PATH` `$HOME/projects/core-simulator/docker/ui-env/ws-token`. `scripts/env/linux-static.sh` SHALL set the same without any Qt library or plugin paths. `scripts/env/macos.sh` SHALL set only the app variables, with `UC_DISPLAY_SCALE` 0.5 and no platform plugin. No script SHALL set `UC_SOCKET_URL`, `UC_RESOURCE_PATH`, `UC_LEGAL_PATH`, `UC_SOUND_EFFECTS_PATH` or `UC_ONBOARDING_PATH`.

#### Scenario: Run target
- **WHEN** a developer runs `make run-linux` after `make linux`
- **THEN** `scripts/env/linux.sh` is sourced in the repository root and the dynamic build is started from its output directory (`make run-linux-static` does the same for the static build)

#### Scenario: Missing binary
- **WHEN** a developer runs `make run-linux` before building
- **THEN** it prints "No binary yet, run: make linux" and fails

#### Scenario: Override before sourcing
- **WHEN** `UC_DISPLAY_SCALE=0.5` is exported before sourcing `scripts/env/linux.sh`
- **THEN** the simulator runs at scale 0.5

#### Scenario: Resource directories not configured
- **WHEN** the simulator runs with the script defaults only
- **THEN** built-in icons are shown, but the custom icon list is empty, page background images are missing, the legal documents are empty, no sound effects play and onboarding is never entered

### Requirement: Platform plugin on a desktop
The app SHALL NOT choose a Qt platform plugin itself. On a Linux desktop it SHALL run with the xcb plugin, which also works on a Wayland session through Xwayland; eglfs is for the device only and aborts on a desktop with "EGLFS: OpenGL windows cannot be mixed with others." because the simulator opens two windows. On macOS the default platform plugin is used.

#### Scenario: Stale eglfs setting
- **WHEN** the simulator is started on a Linux desktop with `QT_QPA_PLATFORM=eglfs`
- **THEN** the app aborts with "EGLFS: OpenGL windows cannot be mixed with others."

#### Scenario: Wayland desktop
- **WHEN** the simulator is started from `scripts/env/linux.sh` on a GNOME Wayland session
- **THEN** it runs through Xwayland with the xcb plugin without further setup
