# app-startup Specification

## Purpose

How the Remote-UI starts: environment configuration, the start-up order from core connection to the first page, loading screens, the onboarding versus main-screen decision, the no-profile and no-page states, and clean shutdown.

## Requirements

### Requirement: Environment configuration
The app SHALL read its configuration from environment variables with these defaults: `UC_MODEL` (`DEV`, `UCR2`, `UCR3`; unset or invalid → `DEV`), `UC_SOCKET_URL` (`ws://127.0.0.1:8080/ws`), `UC_TOKEN_PATH` (none; required to authenticate), `UC_DISPLAY_WIDTH` / `UC_DISPLAY_HEIGHT` (480 / 850, desktop only; devices use the physical screen size), `UC_DISPLAY_SCALE` (desktop only; default 1 on Linux and Windows, 0.5 on macOS), `UC_RESOURCE_PATH` and `UC_LEGAL_PATH` (none; resource and legal text directories), `UC_SOUND_EFFECTS_PATH` (none), `UC_ONBOARDING_PATH` (none; existence of the file selects onboarding), `UC_CONFIG_HOME` (directory of the local `config.ini`), `UC_UI_REQUEST_TIMEOUT` (10000 ms), `UC_WOWLAN` (`true` enables the wake-on-WLAN rows on Remote 3).

#### Scenario: Unknown model
- **WHEN** `UC_MODEL=FOO`, or the former `UC_MODEL=YIO1`
- **THEN** the app runs as the desktop simulator

#### Scenario: Device model
- **WHEN** `UC_MODEL=UCR2` or `UCR3`
- **THEN** the display size is taken from the primary screen and the display variables are ignored

#### Scenario: Display scale default
- **WHEN** the app starts as `DEV` without `UC_DISPLAY_SCALE`
- **THEN** it uses scale 1 on Linux and Windows and scale 0.5 on macOS

### Requirement: Screen orientation per model
On Remote Two the UI SHALL be rendered rotated by −90° (the panel is landscape, the UI portrait), swapping the configured width and height and placing the on-screen keyboard accordingly. On Remote 3 and desktop no rotation is applied.

#### Scenario: Remote Two
- **WHEN** the app starts with `UC_MODEL=UCR2`
- **THEN** the root item is rotated −90° and the reported UI size is portrait

### Requirement: Clean shutdown on signals
On SIGINT, SIGQUIT and SIGTERM the app SHALL quit cleanly (exit code 0) so that systemd stopping the service does not trigger the recovery handler. This SHALL also hold for a signal that arrives during the start-up, before the user interface has finished loading: the stop is remembered and the app quits as soon as it can, instead of carrying on with the start. A second signal SHALL terminate the app right away. The user interface SHALL be torn down before the objects it refers to, so that no screen re-evaluates against destroyed data while it is being shut down: a stop SHALL log no QML type errors and SHALL NOT crash.

#### Scenario: systemd stop
- **WHEN** the process receives SIGTERM
- **THEN** it exits normally

#### Scenario: Log on shutdown
- **WHEN** the app is stopped, idle or with a screen open
- **THEN** the log contains no "Cannot read property ... of null" lines from the user interface, whether the app runs on a desktop or writes to the device journal

#### Scenario: Stop during the start-up
- **WHEN** the service is stopped or restarted one or two seconds after the app started, while it is still loading
- **THEN** the app logs that a termination signal was received and exits with code 0 within the service's stop timeout
- **AND** the service manager does not kill it, the recovery handler does not run and the device does not reboot into the factory UI

#### Scenario: Second signal
- **WHEN** a second termination signal arrives before the app has finished quitting
- **THEN** the app terminates immediately

### Requirement: Fatal QML load failure
If the main QML document fails to load the process SHALL exit with code −1.

#### Scenario: Missing resource
- **WHEN** `qrc:/main.qml` cannot be created
- **THEN** the app exits with −1

### Requirement: Startup sequence
The app SHALL start the core connection immediately, then load in this order: authentication → (in parallel) configuration, API-access state, active profile, all entities in pages of 100, version information and a non-forced software update check → the active profile's details (`get_profile`) → its pages (`get_pages`) → "configuration loaded". The profile list SHALL be requested once all entities are loaded and retried every 2000 ms on failure. Activities found among the entities SHALL be attached to the activity bar after the pages are loaded.

#### Scenario: Normal start
- **WHEN** the core is reachable and the active profile has pages
- **THEN** the pages become visible after profile and pages are loaded and entities are already available to the widgets

#### Scenario: Pages request fails
- **WHEN** `get_pages` fails
- **THEN** "configuration loaded" is still raised with zero pages so the startup screen ends and the no-page screen is shown

### Requirement: Startup loading screen
From process start until "configuration loaded" the UI SHALL show a full black screen with an animated four-arc logo (fade-in after 100 ms) and SHALL swallow all key, mouse and touch input. When loading finishes the arcs fold away over about 1.2 s and the screen fades out over 300 ms, and input is accepted again. During onboarding the screen SHALL be dismissed immediately.

#### Scenario: Core not reachable
- **WHEN** the core never answers
- **THEN** the loading screen stays indefinitely (a connection problem notification appears on top after about 20 s)

#### Scenario: Onboarding device
- **WHEN** the onboarding file exists at start
- **THEN** the loading screen is dismissed right away and the onboarding container is shown

### Requirement: Onboarding versus main container
If the file named by `UC_ONBOARDING_PATH` exists at start the app SHALL run in onboarding mode: the onboarding container is loaded, integration connecting/error indications are suppressed and API-started activities are not opened. Finishing onboarding SHALL delete that file and switch to the main container (or the no-page screen). Without the file the main container is loaded once the configuration is loaded.

#### Scenario: Onboarding finished
- **WHEN** onboarding sets the onboarding flag to false
- **THEN** the file is removed and the main container or no-page screen replaces the onboarding container

### Requirement: No-page screen
When the current profile has zero pages the UI SHALL show the no-page screen instead of the main container, and switch back to the main container as soon as a page exists (page count changes are followed live). For an unrestricted profile the screen shows a "+" with "Tap here to add your first page" which opens the add-page dialog with the keyboard; for a restricted profile it shows "No page found. Ask your administrator to setup pages." without the "+". The status bar and help overlay remain available.

#### Scenario: First page added
- **WHEN** the user creates a page from the no-page screen
- **THEN** the main container with that page replaces the no-page screen

#### Scenario: Last page deleted
- **WHEN** the core reports the last page deleted
- **THEN** the no-page screen is shown

### Requirement: No-profile handling
When the core reports no active profile, or the active profile cannot be loaded, the UI SHALL stop the loading screen, close any open popups and replace the main container by the "add profile" dialog when no profiles exist, otherwise by the profile switcher. This SHALL be the only screen shown for that situation: there is no separate "error loading the profile" screen. The screen SHALL have no back arrow and SHALL NOT be closable with BACK or HOME; the only way out is selecting or adding a profile.

#### Scenario: Fresh core without profiles
- **WHEN** `get_active_profile` fails and the profile list is empty
- **THEN** the add-profile dialog is shown full screen

#### Scenario: Profile exists but none active
- **WHEN** `get_active_profile` fails and profiles exist
- **THEN** the profile switcher is shown full screen

#### Scenario: Profile fails to load
- **WHEN** the active profile cannot be loaded
- **THEN** the same profile selection is shown, and no additional error screen appears over it or over the loading screen

### Requirement: Profile and page changes at runtime
A change of the current profile id (after `switch_profile` or a core event) SHALL reload the profile and its pages. `profile_change` and `page_change` events for the current profile SHALL be applied live (name, icon, restricted flag, page name/image/items, page add/remove). When the current profile is deleted the UI SHALL clear its name, icon and restricted flag, drop its pages from the screen and apply the no-profile handling, so that no pages of a profile that no longer exists stay visible. Failed page operations (add, rename, update, delete) SHALL show "Error <op> page: <message>" and re-synchronise the pages from the core.

#### Scenario: Page renamed in the web configurator
- **WHEN** a `page_change` event arrives for the current profile
- **THEN** the page title updates without reloading

#### Scenario: Current profile deleted from another client
- **WHEN** the core reports the deletion of the profile the remote is showing
- **THEN** its pages disappear and the profile selection is shown, or the add-profile dialog when it was the last profile

### Requirement: Clock
The UI SHALL keep a clock updated every second (coarse timer) in the configured timezone. The status bar shows it as `hh:mm` when 24-hour time is on, otherwise `hh:mm a`.

#### Scenario: Timezone changed
- **WHEN** the timezone setting changes
- **THEN** the displayed time follows within one second

### Requirement: Edit mode
The UI SHALL expose a global edit-mode flag used by the page to reorder its tiles; entering edit mode is done from the page menu, and BACK saves the order.

#### Scenario: Reorder saved
- **WHEN** BACK is pressed in edit mode
- **THEN** the new tile order is sent to the core and edit mode ends

### Requirement: Input keyboard locale
The on-screen keyboard locale SHALL follow the configured UI language.

#### Scenario: Language switched
- **WHEN** the language changes to `de_DE`
- **THEN** the on-screen keyboard uses the German layout
