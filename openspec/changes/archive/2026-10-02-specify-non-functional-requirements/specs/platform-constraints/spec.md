## ADDED Requirements

### Requirement: Target platform and toolchain

The Remote-UI SHALL build as one statically linked aarch64 binary for the Remote Two and
Remote 3 with the public Docker toolchain image (`unfoldedcircle/r2-toolchain-qt-5.15.8-static`,
`make ucr2` or the GitHub workflow), against Qt 5.15 LTS only. The desktop build of the same
code (`make linux`, `make linux-x64`, Qt Creator) is the simulator and MUST NOT diverge in
behaviour beyond what `UC_MODEL` selects. Several Qt 5.15 patch levels MAY be installed side by
side on a development machine and are selected per shell (`scripts/env/qt-version.sh`), the build
defaulting to the newest installed one; continuous integration SHALL build the dynamically linked
desktop build and the unit tests with Qt 5.15.2, so a change MUST NOT rely on behaviour that only
a newer Qt 5.15 patch level has.

#### Scenario: Device build

- **WHEN** the repository is built with the toolchain image
- **THEN** `binaries/linux-arm64/release/remote-ui` is produced, links no shared Qt library, and
  runs on both devices

#### Scenario: Qt 6 API

- **WHEN** a change uses a Qt 6 API, a `QtQuick` 6 import or `qt_add_qml_module`
- **THEN** it does not build for the device and is rejected

#### Scenario: Newer Qt patch level only

- **WHEN** a change works only with a Qt 5.15 patch level newer than the one continuous
  integration builds with
- **THEN** it fails there and is rejected until the build Qt has moved

### Requirement: Release artefacts

A release SHALL contain the statically linked aarch64 device binary, which is the only artefact
that ships in a firmware, and the statically linked Linux x64 desktop simulator, which a user can
run on Ubuntu 24.04, Debian 13 and newer without a Qt installation. Both SHALL be built in public
Docker toolchain images, so that anyone can reproduce them. The experimental Windows x64 desktop
build SHALL NOT be part of a release, SHALL NOT be built by any continuous-integration job and
SHALL be documented as unsupported; development platforms are Linux and macOS. Every binary,
the device binary included, SHALL embed the freely licensed icon font; the device gets the
licensed edition from a file the firmware installs (ADR 0010), never from the build. The macOS
desktop build (`make macos-static`) SHALL be a local developer build, not a release artefact.

#### Scenario: Release build

- **WHEN** a version tag is pushed
- **THEN** the release carries the device binary and the Linux x64 static simulator archive, and
  no Windows artefact

#### Scenario: Pull-request build

- **WHEN** a pull request is built
- **THEN** only the device binary is produced, so the turnaround stays short

#### Scenario: Running a release simulator

- **WHEN** a user unpacks the Linux x64 static archive of a release on Debian 13
- **THEN** the simulator starts against a remote-core without any Qt installed

### Requirement: Supported hardware models and feature parity

The Remote-UI SHALL support the Remote Two (`UCR2`) and the Remote 3 (`UCR3`) on the device and
the desktop simulator (`DEV`) for development. Every feature SHALL be available on both remotes,
except a feature that depends on hardware only one model has, such as the Remote 3 touch slider.
The YIO remote (`YIO1`) is not supported.

#### Scenario: New feature

- **WHEN** a change adds a user-visible feature that does not depend on model-specific hardware
- **THEN** it works on both the Remote Two and the Remote 3

#### Scenario: Model-specific hardware

- **WHEN** a feature needs the touch slider
- **THEN** it is available on the Remote 3 only, and the Remote Two is not required to offer a
  replacement

#### Scenario: YIO model

- **WHEN** a change touches code that only exists for `YIO1`
- **THEN** that code does not need to be kept working and may be removed

### Requirement: Runtime environment

On the device the app SHALL run under systemd on a Buildroot Linux without a window manager,
using the `eglfs` platform plugin on OpenGL ES 2, logging to journald through the Qt logging
categories, and SHALL exit cleanly on `SIGTERM`, `SIGINT` and `SIGQUIT` so that a stop by systemd
is not mistaken for a crash. On a desktop the `xcb` platform plugin is used; `eglfs` aborts there.

#### Scenario: Service stop

- **WHEN** systemd stops the `remote-ui` service
- **THEN** the app quits with exit code 0 and the core's recovery handler is not triggered

#### Scenario: QML load failure

- **WHEN** the main QML file cannot be loaded
- **THEN** the app exits with code -1 so the supervisor can restart or fall back

### Requirement: Display geometry

The UI SHALL render at 480×854 on the Remote Two, whose panel is landscape and requires the UI to
be rotated, and at 480×800 on the Remote 3, on a screen of roughly 47×80 mm viewed at arm's
length. On the desktop the geometry comes from `UC_DISPLAY_WIDTH` / `UC_DISPLAY_HEIGHT` (default
480×850) with `UC_DISPLAY_SCALE` as the scale factor. The default scale SHALL follow the desktop
operating system, so that the simulator looks right on each without configuration: 1 on Linux and
Windows, whose desktops are normally 1x and do their own scaling, and 0.5 on macOS, whose desktop
is a 2x Retina display. `UC_DISPLAY_SCALE` SHALL override that default in both directions.

#### Scenario: Remote Two rotation

- **WHEN** the app starts with `UC_MODEL=UCR2`
- **THEN** the content is rotated so that the UI is portrait on the landscape panel, including
  the on-screen keyboard

#### Scenario: Remote 3

- **WHEN** the app starts with `UC_MODEL=UCR3`
- **THEN** the UI uses the 480×800 screen unrotated

#### Scenario: Desktop development on macOS

- **WHEN** the simulator starts on a macOS Retina display without `UC_DISPLAY_SCALE`
- **THEN** the simulator window shows the UI at its intended size

#### Scenario: Desktop development on Linux or Windows

- **WHEN** the simulator starts on a regular Linux or Windows display without `UC_DISPLAY_SCALE`
- **THEN** the UI fills the simulator window and the button simulator's click areas match the
  keypad picture

### Requirement: Input methods and accessibility

Every screen, popup, drawer and form SHALL be operable by touch and, on the devices, entirely by
the physical buttons, following the input idiom rules of the `key-navigation` capability. The
accessibility requirement of the product SHALL be complete d-pad navigation with a clearly
visible selection. A screen that cannot be left with BACK or HOME on the keypad is a defect.

#### Scenario: Keypad-only walk

- **WHEN** a screen is opened and driven only with the physical buttons
- **THEN** every control is reachable and activatable, the selected control is visibly
  highlighted, and the screen can be left with BACK or HOME

### Requirement: Idle CPU usage

The UI process runs on at most two CPU cores of the device. While the user does not interact with
the remote, it SHALL use as little CPU as possible, and while the display is off it SHALL stay
below 5 % of one core. Short bursts of high CPU usage, such as loading a page or processing a
burst of core events, are acceptable. The display is off in the Low_power mode, where the UI hides
its window and Qt stops rendering; in the Idle mode before it the display is only dimmed and the
UI keeps rendering. While the display is off, animations SHALL stop and rendering SHALL stop
wherever Qt allows it without working against the framework. Processing of Core-API responses and
events SHALL continue, so the UI is up to date when the display turns on again.

#### Scenario: Display off

- **WHEN** the core reports the Low_power mode while the main page with animated content is shown
- **THEN** the window is hidden, no animation runs, the UI process stays below 5 % of one core
  apart from short bursts, and entity changes reported by the core are still applied

#### Scenario: Display dimmed

- **WHEN** the core reports the Idle mode
- **THEN** the UI keeps rendering the dimmed screen and runs no work beyond what the visible
  content needs

#### Scenario: Display on again

- **WHEN** the display turns on after an entity changed while it was off
- **THEN** the first rendered frame already shows the new state

### Requirement: Memory usage

The resident memory of the UI process SHALL NOT exceed 1 GB; the target is below 512 MB. The
budget exists because the Remote Two has 2 GB of memory, which it shares with remote-core, the
other system services and 10 to 20 integrations of 50 to 100 MB each.

#### Scenario: Long-running session

- **WHEN** the UI has run for days with many pages, entities and media artwork
- **THEN** its resident memory stays at or below 1 GB, and a value above 512 MB is treated as a
  defect to investigate

### Requirement: Start-up time

A release SHALL NOT start slower than the previous release, measured from process start to the
first page shown with a running core. Faster start-up SHALL be pursued only when its
implementation cost is reasonable.

#### Scenario: Start-up regression

- **WHEN** a change adds work to the start-up path
- **THEN** the start-up time measured on the device is not longer than before the change

### Requirement: Rendering frame rate

Scrolling, page swipes and other animations SHALL render at 60 frames per second and SHALL NOT
drop below 50 frames per second on either remote.

#### Scenario: Page swipe

- **WHEN** the user swipes between pages of the main screen on a Remote Two or a Remote 3
- **THEN** the animation runs fluidly at 50 to 60 frames per second

### Requirement: Input-to-command latency

The time from the input that triggers a command until its Core-API request is sent over the
WebSocket SHALL be 20 ms or less; this is the target to achieve, so the user never perceives the
remote as slow or sluggish. The input is the touch or the physical button press, and for a key
that also has a long-press action it is the release, which resolves the short press. A change
whose design cannot meet this target with the current architecture SHALL say so in its design.

A control that turns repeated input into one value, such as the brightness, colour temperature,
target temperature and cover position keys, SHALL be exempt: it sends the value once the user
stops, after the shared coalescing delay, so that holding a key does not flood the core with
commands.

#### Scenario: Button press

- **WHEN** the user presses a physical button mapped to an entity command
- **THEN** the command request is sent within 20 ms of the press

#### Scenario: Key with a long-press action

- **WHEN** the user briefly presses a key that also has a long-press action
- **THEN** the short-press command is sent within 20 ms of the release

#### Scenario: Repeated value change

- **WHEN** the user holds the brightness key and releases it
- **THEN** one command with the final value is sent after the coalescing delay

### Requirement: Binary size

The static device binary SHALL be smaller than 100 MB.

#### Scenario: New Qt module or assets

- **WHEN** a change adds a Qt module, fonts, images or other embedded resources
- **THEN** the static `remote-ui` binary built with the toolchain image is still smaller than
  100 MB

### Requirement: Capacity and limits

The UI SHALL handle the limits remote-core enforces without adding limits of its own, and SHALL
keep working when a limit is raised, because these limits are product decisions and not technical
ones. The core enforces 30 profiles, 30 pages per profile, 30 groups per profile, and for an
activity 100 included entities, 15 UI pages and 100 sequence steps, and for a macro 100 included
entities and 100 sequence steps. Docks are not limited; a user typically has one or two and rarely
more than four. The firmware includes 10 integrations, a user may install up to 10 custom
integrations locally, and the number of external integrations is not limited.

The UI SHALL stay responsive with 1000 to 4000 configured entities. Most users have fewer than
100, while power users have several hundred and some more than 1000.

#### Scenario: Many entities

- **WHEN** a profile has several thousand configured entities
- **THEN** entity lists, search and the pages scroll and open without a perceptible delay, and the
  memory budget still holds

#### Scenario: Limit raised by the core

- **WHEN** remote-core raises one of its limits, for example to 50 pages per profile
- **THEN** the UI shows and edits them without a change, because it does not hard-code the limit

#### Scenario: Limit reached

- **WHEN** the user reaches a core limit, for example the 30th page
- **THEN** the core's error is shown; the UI does not pretend a different limit

### Requirement: UI and core ship together

The Remote-UI and remote-core SHALL be released together in one firmware release and are not
updated independently. The UI SHALL only be required to work with the core of its own firmware
release; compatibility code for older core versions is not required.

#### Scenario: Feature needs a new Core-API message

- **WHEN** a change uses a Core-API message that the core of the same firmware release provides
- **THEN** no fallback for older cores is needed

### Requirement: Timings are shared recommended values

The timeouts, delays and animation durations of the UI SHALL be treated as recommended values that
are used consistently, so that the same kind of interaction feels the same everywhere. A change
SHALL reuse the established value for a purpose instead of introducing a new one, and SHALL state
its reason when it deviates. Two screens doing the same thing with different values is a defect.

#### Scenario: New screen with a spinner

- **WHEN** a new screen shows a loading spinner while it waits for the core
- **THEN** it uses the established delay before the spinner appears, not a new value

#### Scenario: Deviating value

- **WHEN** a change needs a different timing than the established one for that purpose
- **THEN** its design says why, and the value is recorded with the others

### Requirement: Unit tests for new logic and bug fixes

New logic SHALL be covered by unit tests wherever a unit test is meaningful, and a bug fix SHALL
come with a unit test that reproduces the defect, to prevent regressions. Code for which a unit
test adds no value is not tested for its own sake.

#### Scenario: New logic

- **WHEN** a change adds C++ logic such as parsing, state handling or a calculation
- **THEN** the change adds unit tests for it to the test targets

#### Scenario: Bug fix

- **WHEN** a change fixes a defect in logic that a unit test can exercise
- **THEN** a unit test fails without the fix and passes with it

### Requirement: Release verification

A release SHALL be tested manually on a Remote Two and on a Remote 3 before it ships, in addition
to the automated checks of CI. This is the developer's responsibility until a written verification
checklist exists; writing that checklist is an open task, and no test is simulator-only by
definition until it says so.

#### Scenario: Release candidate

- **WHEN** a release is prepared
- **THEN** CI is green and the build was run on both hardware models by the developer before the
  release tag is pushed

### Requirement: Text overflow

Text whose length depends on the language or on user-given names SHALL wrap once and then be
truncated with an ellipsis, so it takes at most two lines. Page names and entity names are
expected to be long and wrap on purpose. A single line with truncation SHALL be used where the
design system specifies it, such as title bars and compact rows. Any other line limit SHALL be
specified in the design system; text that deviates without being specified there SHALL be
reviewed.

#### Scenario: Long page name

- **WHEN** a page or entity name is longer than one line
- **THEN** it wraps onto a second line and ends with an ellipsis if it still does not fit

#### Scenario: Title bar

- **WHEN** a title in a title bar is longer than its line
- **THEN** it stays on one line and ends with an ellipsis

#### Scenario: Deviating line limit

- **WHEN** a change limits a text to three lines or more
- **THEN** the limit is specified in the design system, or the change is reviewed for it

### Requirement: Shipped languages

Every language listed in the project's translation list that has a translation SHALL be embedded
in the binary and selectable on the device. `en_US` SHALL be the reference language and the only
translation file edited in the repository; all other languages SHALL come from the translation
service (SimpleLocalize) through its repository sync. Every shipped language SHALL also have its
on-screen keyboard layout, as long as Qt Virtual Keyboard provides a layout for that language.

#### Scenario: Listed language

- **WHEN** a language is in the translation list and has a translation
- **THEN** it can be selected on the device

#### Scenario: Keyboard layout for the language

- **WHEN** the UI language is a shipped language for which Qt Virtual Keyboard provides a layout
- **THEN** the on-screen keyboard shows that language's layout

#### Scenario: Changed English text

- **WHEN** a developer changes a user-visible string
- **THEN** only `en_US.ts` changes in the pull request, and the other languages follow through the
  translation service

### Requirement: Legibility

Font sizes, colours, contrast and the rendering of the d-pad selection SHALL follow the accepted
design system (`docs/design-system.md`). The design system is proposed but not accepted yet;
until it is, a change that sets text sizes, colours or the selection look SHALL raise the
question in its design.

#### Scenario: New screen after acceptance

- **WHEN** a screen is added after the design system has been accepted
- **THEN** its text sizes, colours and selection follow the design system

### Requirement: No secrets in logs

The UI SHALL NOT write secrets to any log, neither from C++ nor from QML. Secrets include access
tokens, passwords, PINs, API keys, the values a user enters into password fields of an
integration or dock setup, and credentials embedded in URLs. Where a logged message contains a
secret, the secret SHALL be replaced by `<redacted>`. The URL of a voice assistant's spoken
answer MAY be logged at debug level: debug output is not stored on the device, whose journal keeps
entries up to priority info (ADR 0013).

#### Scenario: Request with a password

- **WHEN** a request that carries a password or PIN is logged
- **THEN** the log shows `<redacted>` instead of the value

#### Scenario: Integration setup data

- **WHEN** the data a user entered into an integration setup form is logged
- **THEN** the values of password fields and other secrets show `<redacted>`

#### Scenario: Spoken answer URL

- **WHEN** a voice assistant answer arrives with the URL of its audio and debug logging of the
  voice overlay is enabled
- **THEN** the URL is logged at debug level only, which the device's journal does not store

### Requirement: Custom build installation package

A custom UI build SHALL be installable through the core's install API as a tar.gz archive with a
`release.json` (Core-API `CustomRelease` schema) at its root, the binary at `./bin/remote-ui`, and
only `./bin`, `./config` (`UC_CONFIG_HOME`) and `./data` (`UC_DATA_HOME`) as accessible
directories; symlinks are stripped. The install voids the warranty and requires the explicit
confirmation parameter. A custom build SHALL run in its own sandbox, in which the licensed icon
font and the sound effects of the firmware are not available, on purpose: licensed files are not
exposed to third-party binaries. A custom build therefore renders the embedded Free icon font and
plays no sound effects.

#### Scenario: Install and fallback

- **WHEN** a custom archive is uploaded with `void_warranty` confirmed
- **THEN** the UI restarts into the custom binary, and if it fails to start repeatedly the device
  returns to the factory UI

#### Scenario: Custom build and licensed files

- **WHEN** a custom build runs on the device
- **THEN** `UC_ICON_FONT_PATH` names no readable file in its sandbox, the icons render in the Free
  edition, and no sound effect plays

### Requirement: Licensing and published source

All source files SHALL carry the `GPL-3.0-or-later` SPDX header, and the source of every release
SHALL be published in `unfoldedcircle/remote-ui` on GitHub so that the shipped binary can be
rebuilt.

#### Scenario: New source file

- **WHEN** a C++ or QML file is added
- **THEN** it starts with the copyright line and `SPDX-License-Identifier: GPL-3.0-or-later`

### Requirement: Third-party dependencies and assets

New third-party code, libraries and assets SHALL be compatible with Qt under GPL-3.0, statically
linkable, and approved by the lead developer before they are added. A licensed asset that may not
be redistributed SHALL NOT be tracked and SHALL NOT be built into a binary: the repository holds
and every binary embeds only the freely licensed Font Awesome Free icon font, and the firmware
installs the licensed edition as a file that the UI loads at start-up (ADR 0010).

#### Scenario: New library

- **WHEN** a change wants to add a third-party library or asset
- **THEN** its proposal names the license, and the lead developer approves it before it is added

#### Scenario: Tracked icon font

- **WHEN** the repository is checked out and built, anywhere
- **THEN** the embedded icon font is the tracked Font Awesome Free version, and CI fails a commit
  that tracks the licensed font instead

### Requirement: Undecided non-functional requirements are not assumed

The non-functional requirements that are not yet decided SHALL NOT be assumed by an author: the
guarantees in standby beyond the display-off rules and the key handling (until decided, the
behaviour specified in `power-and-battery` and `entity-commands` stands), and which of the
inconsistent timings flagged in the timings inventory of the archived change that introduced this
capability are deliberate (until decided, the specified values stand; a change that touches one
decides it in its design). A change that needs one of them SHALL raise it as an open question for
the maintainer.

#### Scenario: Budget not yet decided

- **WHEN** a change needs a limit this capability does not state
- **THEN** the change's design lists it under Open Questions and the maintainer decides it here
