## MODIFIED Requirements

### Requirement: Simulator main window
On desktop (DEV) the UI SHALL be laid out at `UC_DISPLAY_WIDTH` x `UC_DISPLAY_HEIGHT` logical pixels (default 480 x 850; unset, 0 or non-numeric selects the default), unrotated, with high-DPI scaling enabled and `UC_DISPLAY_SCALE` applied as the global Qt scale factor. The default scale SHALL depend on the desktop operating system: 1 on Linux and Windows, whose desktops are normally 1x and do their own scaling, and 0.5 on macOS, whose desktop is a 2x Retina display; unset, 0 or non-numeric SHALL select that default. The main window SHALL have a fixed size of the UI size divided by the scale factor and the title "Remote Two simulator", with the UI centred in it on a black background. The UI therefore fills the window only at scale 1; at any other scale it occupies that fraction of the window's width and height. Model selection itself is specified in `hardware-platform`.

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
- **WHEN** the simulator starts with `UC_DISPLAY_WIDTH=400`, `UC_DISPLAY_HEIGHT=700` and `UC_DISPLAY_SCALE=1`
- **THEN** the UI is laid out for 400 x 700 in a fixed 400 x 700 window

### Requirement: Emulated buttons and their key events
The button simulator SHALL offer exactly 21 click areas laid over the keypad picture, in a 480 px wide layout: top row BACK (96 x 90), HOME (288 x 90), VOICE (96 x 90); a middle block of 280 px with VOLUME_UP and VOLUME_DOWN stacked on the left (90 x 140 each), a 3 x 3 grid of 96 x 93 areas GREEN, DPAD_UP, YELLOW / DPAD_LEFT, DPAD_MIDDLE, DPAD_RIGHT / RED, DPAD_DOWN, BLUE, and CHANNEL_UP and CHANNEL_DOWN stacked on the right; bottom row MUTE, PREV, PLAY, NEXT, POWER (96 x 90 each). Pressing an area SHALL send the key press event of that physical button to the main window and releasing the mouse button SHALL send the matching key release, so the event takes the same paths as a hardware key (see `key-navigation`). A pressed area SHALL be tinted off-white, fading over 300 ms. The Remote 3 buttons STOP, RECORD and MENU SHALL NOT be emulated. The areas SHALL line up with the picture only while the window is 480 logical pixels wide, i.e. at scale 1, which is the default on Linux and Windows.

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
- **WHEN** the simulator runs with the default scale on a Linux or Windows 1x display
- **THEN** the click areas match the keypad picture

#### Scenario: Scale other than 1
- **WHEN** the simulator runs at scale 0.5 on a 1x display
- **THEN** the click areas are laid out for a 960 px wide window and no longer match the keypad picture

### Requirement: Developer environment scripts
The environment scripts SHALL export each variable only when it is not already set, so a value set before sourcing wins. `scripts/env/linux.sh` SHALL set `QTDIR` to an exported `QTDIR`, else to `$HOME/Qt/<QT_VERSION>/gcc_64`, else to the newest `$HOME/Qt/5.*/gcc_64`, derive `QT_VERSION` from it, prepend `$QTDIR/bin` to `PATH` and `$QTDIR/lib` to `LD_LIBRARY_PATH`, set `QT_PLUGIN_PATH` to `$QTDIR/plugins`, `QT_QPA_PLATFORM` xcb, `UC_MODEL` DEV, `UC_DISPLAY_WIDTH` 480, `UC_DISPLAY_HEIGHT` 850, `UC_DISPLAY_SCALE` 1 and `UC_TOKEN_PATH` `$HOME/projects/core-simulator/docker/ui-env/ws-token`. `scripts/env/linux-static.sh` SHALL set the same without any Qt library or plugin paths. `scripts/env/macos.sh` SHALL set only the app variables, with `UC_DISPLAY_SCALE` 0.5 and no platform plugin. `scripts/env/windows.cmd` SHALL set the same app variables with `UC_DISPLAY_SCALE` 1, additionally `UC_SOCKET_URL` `ws://127.0.0.1:8080/ws`, print what it set, and start the Windows executable, by default the one in the cross-compile output directory or the one named as its first argument. No script other than `scripts/env/windows.cmd` SHALL set `UC_SOCKET_URL`, and no script SHALL set `UC_RESOURCE_PATH`, `UC_LEGAL_PATH`, `UC_SOUND_EFFECTS_PATH` or `UC_ONBOARDING_PATH`.

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

## ADDED Requirements

### Requirement: Qt version selection for a desktop build
A desktop developer SHALL be able to keep several Qt 5.15 installations side by side and pick one per shell with `. scripts/env/qt-version.sh [version|prefix]`, which selects that Qt for `make`, CMake and Qt Creator and removes a previously selected Qt from the shell first, so it can be sourced again. Without a selection the build SHALL use the newest installed Qt, and a version named on the `make` command line SHALL win over a selection made in the shell. Switching the Qt of the unit-test build SHALL not require a manual clean. Continuous integration SHALL keep building the dynamically linked desktop build and the unit tests with Qt 5.15.2, so a change MUST NOT depend on behaviour that only a newer Qt 5.15 patch level has.

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
