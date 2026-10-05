## MODIFIED Requirements

### Requirement: Environment configuration
The app SHALL read its configuration from environment variables with these defaults: `UC_MODEL` (`DEV`, `YIO1`, `UCR2`, `UCR3`; unset or invalid → `DEV`), `UC_SOCKET_URL` (`ws://127.0.0.1:8080/ws`), `UC_TOKEN_PATH` (none; required to authenticate), `UC_DISPLAY_WIDTH` / `UC_DISPLAY_HEIGHT` (480 / 850, desktop only; devices use the physical screen size), `UC_DISPLAY_SCALE` (desktop only; default 1 on Linux and Windows, 0.5 on macOS), `UC_RESOURCE_PATH` and `UC_LEGAL_PATH` (none; resource and legal text directories), `UC_SOUND_EFFECTS_PATH` (none), `UC_ONBOARDING_PATH` (none; existence of the file selects onboarding), `UC_CONFIG_HOME` (directory of the local `config.ini`), `UC_UI_REQUEST_TIMEOUT` (10000 ms), `UC_WOWLAN` (`true` enables the wake-on-WLAN rows on Remote 3).

#### Scenario: Unknown model
- **WHEN** `UC_MODEL=FOO`
- **THEN** the app runs as the desktop simulator

#### Scenario: Device model
- **WHEN** `UC_MODEL=UCR2` or `UCR3`
- **THEN** the display size is taken from the primary screen and the display variables are ignored

#### Scenario: Display scale default
- **WHEN** the app starts as `DEV` without `UC_DISPLAY_SCALE`
- **THEN** it uses scale 1 on Linux and Windows and scale 0.5 on macOS
