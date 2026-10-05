## MODIFIED Requirements

### Requirement: Configuration loaded from the core on every connection
On every successful authentication the UI SHALL request the complete configuration (`get_config`), the API-access state (`get_api_access`) and the active profile (`get_active_profile`). A failed `get_config` SHALL show the warning notification "Error while loading configuration. Trying again." and retry after 2000 ms until it succeeds. The response SHALL populate the button, display, device, haptic, localization, network, power-saving, software-update, sound and voice-control settings. Until the first configuration response arrives the UI SHALL show and compare against fixed values: display and button brightness 50, wakeup sensitivity high, WiFi enabled, sleep timeout 60 s, display off timeout 30 s, and every other core-backed toggle off and every other number 0; the first response SHALL replace all of them.

#### Scenario: Config request fails
- **WHEN** `get_config` is answered with an error or times out
- **THEN** the notification is shown and the request is repeated 2 s later

#### Scenario: Active profile unknown
- **WHEN** `get_active_profile` fails
- **THEN** the UI enters the no-profile flow (profile selection or creation) instead of loading pages

#### Scenario: Before the first configuration
- **WHEN** a settings page is opened, or a setting is compared, before the first `get_config` response arrived
- **THEN** it shows the values listed above, the same on every build and architecture, never an arbitrary value
- **AND** when the response arrives every setting takes the core's value

### Requirement: Local UI preferences
The following settings SHALL be stored locally in `config.ini` under `UC_CONFIG_HOME`, apply immediately without a core round-trip and default as listed: "Inverted button behaviour" (false; "Inverts button functions on the main screen: short press to open the control screen, long press to quick toggle."), "Show battery percentage" (false), "Show battery indicator everywhere" (false), "Activities on pages" (true), "Open activities started with the API" (false), "Zoom media image" (false), "Coverflow in media browser" (false), "Touch slider" enabled (true), touch slider gains volume 0.4, brightness 1.2, cover position 1.2, seek 1.0 (each 0.1–2.0, step 0.1), retry window 2 s. A value changed right before the app stops SHALL still be written to `config.ini` when the app exits.

#### Scenario: Fresh install
- **WHEN** no `config.ini` exists
- **THEN** the UI page shows the defaults above

#### Scenario: Touch slider disabled
- **WHEN** "Touch slider" is off
- **THEN** the gain sliders are disabled and swiping the hardware slider does nothing anywhere

#### Scenario: Changed right before shutdown
- **WHEN** a local preference is changed and the app is stopped (service stop, restart or shutdown of the remote) immediately afterwards
- **THEN** the next start shows the changed value
