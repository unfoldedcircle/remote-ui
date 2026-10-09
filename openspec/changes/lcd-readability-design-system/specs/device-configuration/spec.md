## MODIFIED Requirements

### Requirement: Local UI preferences
The following settings SHALL be stored locally in `config.ini` under `UC_CONFIG_HOME`, apply immediately without a core round-trip and default as listed: "Inverted button behavior" (false; "Inverts button functions on the main screen: short press to open the control screen, long press to quick toggle."), "Show battery percentage" (false), "Show battery indicator everywhere" (false), "Activities on pages" (true), "Open activities started with the API" (false), "Zoom media image" (false), "Coverflow in media browser" (false), "Touch slider" enabled (true), touch slider gains volume 0.4, brightness 1.2, cover position 1.2, seek 1.0 (each 0.1–2.0, step 0.1), retry window 2 s. A value changed right before the app stops SHALL still be written to `config.ini` when the app exits.

#### Scenario: Fresh install
- **WHEN** no `config.ini` exists
- **THEN** the UI page shows the defaults above

#### Scenario: Touch slider disabled
- **WHEN** "Touch slider" is off
- **THEN** the gain sliders are disabled and swiping the hardware slider does nothing anywhere

#### Scenario: Changed right before shutdown
- **WHEN** a local preference is changed and the app is stopped (service stop, restart or shutdown of the remote) immediately afterwards
- **THEN** the next start shows the changed value
