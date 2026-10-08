# desktop-simulator Specification

## Purpose

How the same code runs on a desktop as a device simulator: the DEV model, the button simulator window, which hardware paths are inert, why device models on a desktop are not supported, and what depends on the Remote-Core Simulator. The simulator is built from the same code as the device binary; only a few hardware features are disabled or simulated, selected by the model.

## Requirements

### Requirement: Simulator main window
The desktop simulator is the model `DEV` (`UC_MODEL=DEV`, also the default); it simulates a Remote Two: a Remote Two sized window and the Remote Two keypad in the button simulator. A Remote 3 simulation is not implemented. On desktop (DEV) the UI SHALL be laid out at `UC_DISPLAY_WIDTH` x `UC_DISPLAY_HEIGHT` logical pixels (default 480 x 850; unset, 0 or non-numeric selects the default), unrotated, with high-DPI scaling enabled and `UC_DISPLAY_SCALE` applied as the global Qt scale factor. The default scale SHALL depend on the desktop operating system: 1 on Linux and Windows, whose desktops are normally 1x and do their own scaling, and 0.5 on macOS, whose desktop is a 2x Retina display; unset, 0 or non-numeric SHALL select that default. The main window SHALL have a fixed size of the UI size divided by the scale factor and the title "Remote Two simulator", with the UI centred in it on a black background. The UI therefore fills the window only at scale 1; at any other scale it occupies that fraction of the window's width and height. Other sizes than the default are for testing only: the layout is designed for the device panels and is not responsive, so at another size elements can be cut off or overlap. Model selection itself is specified in `hardware-platform`.

#### Scenario: Scale 1 on a regular display
- **WHEN** the simulator starts with `UC_DISPLAY_SCALE=1` and no size variables on a 1x display
- **THEN** a non-resizable 480 x 850 window opens and the UI fills it exactly

#### Scenario: Default scale on a regular display
- **WHEN** the simulator starts without `UC_DISPLAY_SCALE` on a Linux or Windows 1x display
- **THEN** a non-resizable 480 x 850 window opens and the UI fills it exactly

#### Scenario: Default scale on macOS
- **WHEN** the simulator starts without `UC_DISPLAY_SCALE` on a macOS 2x Retina display
- **THEN** the UI is drawn at half its logical size, which is the physical size of the device panel

#### Scenario: Scale explicitly halved
- **WHEN** the simulator starts with `UC_DISPLAY_SCALE=0.5` on a 1x display
- **THEN** the window is 480 x 850 screen pixels and the UI is drawn at 240 x 425 in its centre
- **AND** the on-screen keyboard is visible below the UI in its parked position

#### Scenario: Custom size
- **WHEN** the simulator starts with `UC_DISPLAY_WIDTH=400`, `UC_DISPLAY_HEIGHT=700` and `UC_DISPLAY_SCALE=1` for a test
- **THEN** the UI is laid out for 400 x 700 in a fixed 400 x 700 window, and screens are not adapted to the size: elements can be cut off or overlap

### Requirement: Button simulator window
On the desktop model `DEV`, the only model that does not show regulatory information, the UI SHALL open a second window titled "Button simulator" with a black background, fixed at the main window's width and 0.95 times that width in height, showing a picture of the Remote Two keypad. The window SHALL be placed 60 px below the main window, left-aligned with it, or, when the main window's height plus the simulator's height exceed the available desktop height, directly to the right of the main window aligned with its top edge. Its content SHALL only be loaded while the window is visible. On Remote Two and Remote 3 the window SHALL stay hidden and empty.

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

### Requirement: Desktop keyboard as keypad
When the UI runs on a desktop (any model), key events from the computer keyboard SHALL be handled like physical buttons whenever their key code belongs to the button map: the arrow keys act as DPAD_UP / DPAD_DOWN / DPAD_LEFT / DPAD_RIGHT, Return as DPAD_MIDDLE, Home as HOME, F3 as VOICE and F4 as MENU. On the desktop model `DEV`, Escape SHALL act as BACK: every Escape key press and release SHALL be replaced by the key event of the BACK button, with the same press or release and auto-repeat flag, so that the button-navigation handlers and the focused control receive BACK exactly as from a device, and no control SHALL receive the Escape event, including a popup that would close on Escape. Backspace and the numeric keypad Enter SHALL NOT act as any button. A held keyboard key auto-repeats like a held hardware key. Any other key only reaches the focused control (e.g. typing into a text field).

#### Scenario: Arrow keys
- **WHEN** the user presses the Down arrow key on the desktop keyboard
- **THEN** the UI reacts as to DPAD_DOWN and the keypad selection highlights become visible

#### Scenario: Escape
- **WHEN** the user presses Escape on a settings page in `DEV`
- **THEN** the UI reacts as to BACK and the page is left

#### Scenario: Escape on a popup that closes on Escape
- **WHEN** the user presses Escape in `DEV` while a popup is open whose close policy includes closing on Escape
- **THEN** the popup does not close because of Escape; it reacts to BACK as on a device

#### Scenario: Held Escape
- **WHEN** the user holds Escape in `DEV`
- **THEN** the UI reacts as to a held BACK button: auto-repeat presses until the key is released, then one BACK release

#### Scenario: Backspace
- **WHEN** the user presses Backspace on a settings page
- **THEN** the page is not left

#### Scenario: Held arrow key
- **WHEN** the user holds the Down arrow key on a list that supports repeat
- **THEN** the selection keeps moving until the key is released

### Requirement: Keyboard focus with the second window
The button simulator window SHALL be created as a window that never accepts focus, so that clicking it leaves the main window active and the main window keeps its active focus item. Focus-chain navigation (keyboard focus highlight, up/down focus hand-over, Return on a focused control) SHALL therefore work on desktop (DEV) as on the device. The focus chain SHALL only react while the main window is the active desktop window; the button ownership path reacts regardless.

#### Scenario: Clicking the simulator
- **WHEN** the user clicks DPAD_DOWN in the button simulator on a focus-chain settings page
- **THEN** the main window stays active and the focus highlight moves to the next control

#### Scenario: Main window inactive
- **WHEN** another desktop application is active and a simulator button is clicked
- **THEN** screens driven by button ownership still react, but focus-chain screens show no highlight and do not move

### Requirement: Hardware behaviour on desktop
On desktop (DEV) the UI SHALL run without device hardware: haptic effects SHALL be silently dropped; the touch slider SHALL never produce input, although slider overlays are armed as on Remote 3; touch and mouse input SHALL never be blocked by the low-power mode, and the window SHALL stay shown in every power mode, since nothing on a desktop wakes the core simulator from Low_power; no software dimming overlay SHALL be drawn; the UI SHALL not be rotated. WiFi, Bluetooth, power, battery and system information SHALL come only from the core. Where screens differ per model, desktop SHALL follow Remote 3: the "WiFi band" selector and the "WPA3 Personal" security option are offered, media artwork on the media player screens is 60 px shorter than its width, and the wake-on-WLAN rows are shown only with `UC_WOWLAN=true`. The model number SHALL read "DEV" once the core answered the system information request, the onboarding remote name default SHALL be "Remote Two", and the About page SHALL still list the Regulatory entry.

#### Scenario: Haptic feedback on desktop
- **WHEN** the user taps a button on desktop with haptic feedback enabled
- **THEN** nothing is written to any device and nothing is logged

#### Scenario: Low power on desktop
- **WHEN** the core reports the LOW_POWER mode on desktop
- **THEN** the window stays shown and mouse clicks on the UI are still accepted

#### Scenario: WiFi settings on desktop
- **WHEN** the user opens the WiFi settings on desktop
- **THEN** the "WiFi band" row is shown, as on Remote 3

### Requirement: Device models on a desktop
Running with `UC_MODEL=UCR2` or `UCR3` on a desktop is not a supported feature and is not recommended; it only exists because the model selection is the same code as on the device. Development and testing on a desktop use `DEV`. When `UC_MODEL` is `UCR2` or `UCR3` on a desktop the UI SHALL open only the main window, sized and fixed to the full geometry of the primary screen, with no desktop scale factor applied and `UC_DISPLAY_WIDTH`, `UC_DISPLAY_HEIGHT` and `UC_DISPLAY_SCALE` ignored. With `UCR2` the width and height SHALL be swapped and the UI rotated by -90 degrees as on the device. The device drivers SHALL be active: without a haptic device at `UC_HAPTIC_DEV_PATH` every haptic effect logs "Failed to write to haptic device"; with `UCR3` and no device at `UC_TOUCHSLIDER_DEV_PATH` the touch slider driver retries opening it every 1 s and logs one warning until the device appears, and again only after the device was found and has gone missing once more. Input then comes only from mouse, touch and the computer keyboard.

#### Scenario: Remote 3 on a desktop
- **WHEN** the app starts with `UC_MODEL=UCR3` on a 1920 x 1080 screen
- **THEN** one fixed 1920 x 1080 window opens with an unrotated UI and no button simulator

#### Scenario: Remote Two on a desktop
- **WHEN** the app starts with `UC_MODEL=UCR2` on a 1920 x 1080 screen
- **THEN** the UI is laid out for 1080 x 1920 and shown rotated by -90 degrees

#### Scenario: Missing touch slider device
- **WHEN** the app runs with `UC_MODEL=UCR3` and `UC_TOUCHSLIDER_DEV_PATH` does not exist
- **THEN** a "Touch slider device does not exist" warning is logged once, not once per second, while the driver keeps retrying every 1 s

### Requirement: Remote-Core Simulator dependency
On desktop the UI SHALL obtain all configuration, entities and state from a remote-core at `UC_SOCKET_URL` (default `ws://127.0.0.1:8080/ws`), normally the Remote-Core Simulator started with docker-compose, authenticating with the token file named by `UC_TOKEN_PATH`, which for the simulator is `$CORE_SIMULATOR_PATH/docker/ui-env/ws-token`. Connection and authentication behaviour are specified in `core-connection`; the startup screen in `app-startup`.

#### Scenario: Simulator not running
- **WHEN** the app starts on desktop while no core listens on `UC_SOCKET_URL`
- **THEN** the startup screen stays and a connection problem is reported

#### Scenario: Token path not set
- **WHEN** the app starts on desktop without `UC_TOKEN_PATH`
- **THEN** the UI never becomes authenticated and the startup screen stays

### Requirement: Desktop verification limits
A desktop run SHALL NOT be treated as verification of behaviour that needs device hardware or core functions the Remote-Core Simulator does not provide: haptics, the touch slider, battery and charging, power modes and suspend/resume, WiFi and Bluetooth hardware, the physical keypad (only the Remote Two keys are emulated), mDNS discovery of docks and integrations, and installing custom integrations. Everything else runs the same code as on the device and behaves the same against the Remote-Core Simulator.

#### Scenario: Keypad verification
- **WHEN** a change affects d-pad navigation
- **THEN** it is walked with the keypad on a device, or with the computer keyboard or a held simulator button in `DEV`, not only by clicking the button simulator

### Requirement: Developer environment scripts
The recommended way to build, run, test and clean a desktop build SHALL be `make` in the repository root with a target (`make linux`, `make run-linux`, `make test`, `make clean`, …); `make` without a target SHALL print the list of all targets with a one-line description each. The run targets source the matching environment script. The environment scripts SHALL export each variable only when it is not already set, so a value set before sourcing wins. `scripts/env/linux.sh` SHALL set `QTDIR` to an exported `QTDIR`, else to `$HOME/Qt/<QT_VERSION>/gcc_64`, else to the newest `$HOME/Qt/5.*/gcc_64`, derive `QT_VERSION` from it, prepend `$QTDIR/bin` to `PATH` and `$QTDIR/lib` to `LD_LIBRARY_PATH`, set `QT_PLUGIN_PATH` to `$QTDIR/plugins`, `QT_QPA_PLATFORM` xcb, `UC_MODEL` DEV, `UC_DISPLAY_WIDTH` 480, `UC_DISPLAY_HEIGHT` 850, `UC_DISPLAY_SCALE` 1 and `UC_TOKEN_PATH` `$HOME/projects/core-simulator/docker/ui-env/ws-token`. `scripts/env/linux-static.sh` SHALL set the same without any Qt library or plugin paths. `scripts/env/macos.sh` SHALL set only the app variables, with `UC_DISPLAY_SCALE` 0.5 and no platform plugin; it needs no Qt paths, because the static app bundle contains Qt and the shared one finds the Qt frameworks of the installation it was built with by itself. `scripts/env/windows.cmd` SHALL set the same app variables with `UC_DISPLAY_SCALE` 1, additionally `UC_SOCKET_URL` `ws://127.0.0.1:8080/ws`, print what it set, and start the Windows executable, by default the one in the cross-compile output directory or the one named as its first argument. No script other than `scripts/env/windows.cmd` SHALL set `UC_SOCKET_URL`, and no script SHALL set `UC_RESOURCE_PATH`, `UC_LEGAL_PATH`, `UC_SOUND_EFFECTS_PATH` or `UC_ONBOARDING_PATH`. On macOS `make run-macos` and `make run-macos-static` SHALL source `scripts/env/macos.sh` and start the executable inside the app bundle of the matching build, and fail with a hint to the build target when that bundle does not exist yet.

#### Scenario: List of targets
- **WHEN** a developer runs `make` in the repository root without a target
- **THEN** every build, run, test and clean target is listed with its description, and nothing is built

#### Scenario: Run target
- **WHEN** a developer runs `make run-linux` after `make linux`
- **THEN** `scripts/env/linux.sh` is sourced in the repository root and the dynamic build is started from its output directory (`make run-linux-static` does the same for the static build)

#### Scenario: Missing binary
- **WHEN** a developer runs `make run-linux` before building
- **THEN** it prints "No binary yet, run: make linux" and fails

#### Scenario: Starting the Windows build
- **WHEN** a developer runs `scripts\env\windows.cmd` in a command window
- **THEN** the executable starts with the default environment, the values used are printed, and a failure to start points at the graphics libraries the guide describes

#### Scenario: Override before sourcing
- **WHEN** `UC_DISPLAY_SCALE=0.5` is exported before sourcing `scripts/env/linux.sh`
- **THEN** the simulator runs at scale 0.5

#### Scenario: Resource directories not configured
- **WHEN** the simulator runs with the script defaults only
- **THEN** built-in icons are shown, but the custom icon list is empty, page background images are missing, the legal documents are empty, no sound effects play and onboarding is never entered

#### Scenario: Run target on macOS
- **WHEN** a developer runs `make run-macos-static` after `make macos-static` on an Apple Silicon Mac
- **THEN** `scripts/env/macos.sh` is sourced in the repository root and the app in `binaries/macOS-arm64-static/Remote UI.app` is started at scale 0.5 (`make run-macos` does the same for the shared build)

#### Scenario: Missing app bundle on macOS
- **WHEN** a developer runs `make run-macos-static` before building
- **THEN** it prints "No app yet, run: make macos-static" and fails

### Requirement: Platform plugin on a desktop
The app SHALL NOT choose a Qt platform plugin itself. On a Linux desktop it SHALL run with the xcb plugin, which also works on a Wayland session through Xwayland; eglfs is for the device only and aborts on a desktop with "EGLFS: OpenGL windows cannot be mixed with others." because the simulator opens two windows. On macOS and on Windows the platform's default plugin is used; on Windows Qt chooses the graphics stack itself, preferring the system OpenGL, then ANGLE when its libraries sit next to the executable, then the software renderer, and `QT_OPENGL` forces one of them.

#### Scenario: Stale eglfs setting
- **WHEN** the simulator is started on a Linux desktop with `QT_QPA_PLATFORM=eglfs`
- **THEN** the app aborts with "EGLFS: OpenGL windows cannot be mixed with others."

#### Scenario: Wayland desktop
- **WHEN** the simulator is started from `scripts/env/linux.sh` on a GNOME Wayland session
- **THEN** it runs through Xwayland with the xcb plugin without further setup

#### Scenario: Windows without OpenGL drivers
- **WHEN** the Windows build is started in a virtual machine without OpenGL drivers and without the ANGLE libraries beside it
- **THEN** it does not open a window, and placing those libraries next to the executable, or forcing a renderer with `QT_OPENGL`, makes it start

### Requirement: Qt version selection for a desktop build
A desktop developer SHALL be able to keep several Qt 5.15 installations side by side and pick one per shell with `. scripts/env/qt-version.sh [version|prefix]`, which selects that Qt for `make`, CMake and Qt Creator and removes a previously selected Qt from the shell first, so it can be sourced again. Without a selection the build SHALL use the newest installed Qt, and a version named on the `make` command line SHALL win over a selection made in the shell. Switching the Qt of the unit-test build SHALL not require a manual clean. A shared Qt SHALL have priority: without a version the script SHALL take the newest static Qt only when no shared Qt is installed at all, and with a version it SHALL take the static installation of that version only when the version has no shared one. A selected static Qt, also one given by path, SHALL be exported as `QTDIR_STATIC` too, so that the static build targets use it, and `make linux`, `make macos` and `make test` SHALL refuse a static Qt with an error that names the installation guide. The script SHALL work when sourced from bash and from zsh. Continuous integration SHALL keep building the dynamically linked desktop build and the unit tests with Qt 5.15.2, so a change MUST NOT depend on behaviour that differs between the Qt 5.15 patch levels in use. The installations SHALL live in `~/Qt/<version>/` under a directory name that follows the platform: `gcc_64` on Linux and `clang_64` on macOS for the shared Qt, with a `-static` suffix for the static Qt. On macOS the build defaults and the version script SHALL look for the `clang_64` installations, so selecting a version or building without a selection works there as on Linux.

#### Scenario: Selecting a version
- **WHEN** a developer sources the version script with a version that is installed
- **THEN** the shell reports that Qt and every following build, test and Qt Creator run uses it

#### Scenario: No selection
- **WHEN** a developer builds the desktop simulator in a shell without a selection
- **THEN** the newest installed Qt 5.15 is used and the build prints which version that is

#### Scenario: Version on the command line
- **WHEN** a developer names another version on the `make` command line while a different Qt is selected in the shell
- **THEN** the named version is used for that build

#### Scenario: Switching the test build
- **WHEN** the unit tests were built against one Qt and are built again after selecting another
- **THEN** the stale build directory is reset automatically and the tests build and run against the newly selected Qt

#### Scenario: Selecting a version on macOS
- **WHEN** a developer on a Mac sources the version script with `5.15.19` and a shared Qt is installed in `~/Qt/5.15.19/clang_64`
- **THEN** that installation is selected and reported

#### Scenario: No selection on macOS
- **WHEN** a developer on a Mac runs `make macos-static` without a selection and with only `~/Qt/5.15.19/clang_64-static` installed
- **THEN** the build uses that static Qt

#### Scenario: Only a static Qt installed
- **WHEN** a developer on a Mac set up with the macOS guide, with only `~/Qt/5.15.19/clang_64-static`, sources the version script with or without `5.15.19`
- **THEN** that static Qt is selected, exported as `QTDIR` and `QTDIR_STATIC`, and the script reports that `make macos` and `make test` need a shared Qt

#### Scenario: Unit tests against a static Qt
- **WHEN** a developer runs `make test` while `QTDIR` points at a static Qt
- **THEN** it stops before configuring with an error that the Qt is not a shared build

#### Scenario: Sourcing from zsh
- **WHEN** a developer sources the version script without a version from zsh and only one of `gcc_64` and `clang_64` exists
- **THEN** the newest installed Qt is selected without a shell error

### Requirement: Static Linux x64 desktop build
The desktop simulator SHALL be buildable as a statically linked Linux x64 binary in a public Docker toolchain image with `make linux-x64`, without any Qt installed on the machine, and SHALL be startable from that output with `make run-linux-x64`. Every release SHALL carry that binary as an additional asset next to the device build; it SHALL be built for release builds only, not for pull requests. The binary SHALL run on Ubuntu 24.04, Debian 13 and newer without a Qt installation, needing only the documented system libraries, and SHALL behave exactly like a locally built simulator of the same sources.

#### Scenario: Building without Qt
- **WHEN** a developer with Docker but without a Qt installation runs `make linux-x64`
- **THEN** the image is pulled if needed and a self-contained simulator binary is produced

#### Scenario: Running a released simulator
- **WHEN** a user downloads the Linux x64 static archive of a release and starts it on Debian 13
- **THEN** the simulator runs against a remote-core at `UC_SOCKET_URL` without installing Qt

#### Scenario: Pull-request build
- **WHEN** a pull request is built
- **THEN** only the device build runs and no Linux x64 desktop artefact is produced

### Requirement: Experimental Windows x64 desktop build
The desktop simulator SHALL be cross-compilable on Linux into a self-contained Windows x64 executable in a public Docker toolchain image with `make windows-x64`. This build SHALL be labelled experimental and unsupported: development happens on Linux and macOS, no continuous-integration job builds it, and it is not part of a release. The build SHALL offer an opt-in console subsystem, so that a developer who enables it sees the same log categories as on Linux in the command window the executable was started from, a Windows GUI-subsystem build having no usable standard error. The executable SHALL be startable with `scripts\env\windows.cmd`, which sets the app environment, prints the values it used and keeps the command window open when the app exits with an error. Where the touch slider device cannot be opened, as on Windows, the touch slider SHALL be reported as unavailable instead of failing.

#### Scenario: Cross-compiling on Linux
- **WHEN** a developer runs `make windows-x64` on Linux with Docker
- **THEN** a self-contained Windows executable is produced that imports only Windows system libraries

#### Scenario: Running on Windows
- **WHEN** the executable is started from a command window with the start script
- **THEN** the simulator opens and connects to a remote-core at `UC_SOCKET_URL`, and with the console subsystem enabled its log appears in that command window

#### Scenario: Not a supported platform
- **WHEN** the Windows build misbehaves
- **THEN** it is not a release blocker, because the build is experimental and verified on Linux and macOS only

#### Scenario: Touch slider on Windows
- **WHEN** the app runs with `UC_MODEL=UCR3` on Windows
- **THEN** the touch slider is reported as not available instead of the app failing to open the device

### Requirement: macOS desktop build
The desktop simulator SHALL be buildable natively on macOS, on Intel (x86_64) and Apple Silicon (arm64) Macs, with a Qt 5.15.19 compiled from the published source archive, using only Xcode or its command line tools: no Homebrew or MacPorts package SHALL be needed, and none SHALL end up in the app. `make macos-static` SHALL build with a static Qt a self-contained `Remote UI.app` that needs no Qt installation and no library path at runtime and links only system frameworks and libraries; `make macos` SHALL build the same app with a shared Qt for development, which links the Qt frameworks of the local installation and therefore does not run on another Mac. Each target SHALL refuse a Qt of the other link mode. The app SHALL be written to `binaries/macOS-x64-static/` or `binaries/macOS-x64/` on an Intel Mac and `binaries/macOS-arm64-static/` or `binaries/macOS-arm64/` on Apple Silicon, built for the architecture of the Mac it was built on only; there is no universal binary. The oldest supported macOS SHALL be the deployment target the Qt sources were prepared with, macOS 11.0 by default, and the app bundle SHALL declare that same version as its minimum system version. TLS connections SHALL use the system Secure Transport framework. `make test` SHALL build the unit tests for the architecture of the Mac, like the Qt built from source, so that they link against it; an x86_64-only Qt on Apple Silicon SHALL be usable by passing `-D CMAKE_OSX_ARCHITECTURES=x86_64` when `test/build` is configured, the tests then run under Rosetta, and no test CMakeLists SHALL set the architecture itself. The macOS build SHALL NOT be built by continuous integration and SHALL NOT be part of a release; the app is neither signed nor notarized.

#### Scenario: Static build on Apple Silicon
- **WHEN** a developer with a static Qt 5.15.19 built from source runs `make macos-static` on an Apple Silicon Mac
- **THEN** `binaries/macOS-arm64-static/Remote UI.app` is produced, links no Qt library and nothing outside the system frameworks and libraries, and starts on that Mac without any Qt or library path set

#### Scenario: Static build on an Intel Mac
- **WHEN** the same build runs on an Intel Mac
- **THEN** the app is written to `binaries/macOS-x64-static/Remote UI.app` and contains x86_64 code only

#### Scenario: Shared build for development
- **WHEN** a developer runs `make macos` with a shared Qt in `~/Qt/5.15.19/clang_64`
- **THEN** `Remote UI.app` is written to `binaries/macOS-<x64|arm64>/` and runs on that Mac without environment variables, loading the Qt frameworks from that installation

#### Scenario: Wrong Qt link mode
- **WHEN** `make macos-static` is pointed at a shared Qt
- **THEN** it stops before building with an error that names the guide for installing a static Qt

#### Scenario: Minimum macOS version
- **WHEN** the app is built with a Qt prepared for the default deployment target
- **THEN** the app bundle declares macOS 11.0 as its minimum system version and the compiler targets macOS 11.0 without libc++ platform warnings

#### Scenario: Copying the app to another Mac
- **WHEN** a user copies a static `Remote UI.app` to another Mac and opens it
- **THEN** Gatekeeper refuses it until it is opened once with right click, Open, or its quarantine attribute is removed, after which it runs against a remote-core at `UC_SOCKET_URL`

#### Scenario: Release build
- **WHEN** a version tag is built
- **THEN** no macOS artefact is produced

#### Scenario: Unit tests on Apple Silicon
- **WHEN** a developer runs `make test` on an Apple Silicon Mac with a shared Qt 5.15.19 built from source
- **THEN** the unit tests are built for arm64, link against that Qt and run
