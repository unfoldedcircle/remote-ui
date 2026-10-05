## ADDED Requirements

### Requirement: Configuration loaded from the core on every connection
On every successful authentication the UI SHALL request the complete configuration (`get_config`), the API-access state (`get_api_access`) and the active profile (`get_active_profile`). A failed `get_config` SHALL show the warning notification "Error while loading configuration. Trying again." and retry after 2000 ms until it succeeds. The response SHALL populate the button, display, device, haptic, localization, network, power-saving, software-update, sound and voice-control settings.

#### Scenario: Config request fails
- **WHEN** `get_config` is answered with an error or times out
- **THEN** the notification is shown and the request is repeated 2 s later

#### Scenario: Active profile unknown
- **WHEN** `get_active_profile` fails
- **THEN** the UI enters the no-profile flow (profile selection or creation) instead of loading pages

### Requirement: Configuration changes pushed by the core are applied live
A `configuration_change` event SHALL be applied per section present in `new_state` (`button`, `display`, `device`, `haptic`, `localization`, `network`, `software_update`, `power_saving`, `sound`, `voice`); sections not present are left unchanged. Applied values SHALL update every open settings page and dependent behaviour (sound effects, display dimming) without a restart. Localization sections are applied field by field (see the localization capability).

#### Scenario: Web configurator changes brightness
- **WHEN** the core reports a `display` section with a new brightness
- **THEN** the Display settings slider and the screen brightness follow immediately

### Requirement: Settings are written through the core and applied on success
Every core-backed setting SHALL be sent to the core first and only be applied locally after a success response (200/201). On failure the value SHALL stay unchanged and a warning notification "Error setting <setting>: <core message>" SHALL be shown. Setting a value equal to the current one SHALL NOT send a request. Settings that share one core command SHALL always be sent together with the current values of their siblings (display brightness + auto brightness via `set_display_cfg`; button brightness + auto via `set_button_cfg`; sound enabled + volume via `set_sound_cfg`; wakeup sensitivity + display-off + standby via `set_power_saving_cfg`; check-for-updates + auto-update via `set_software_update_cfg`; Bluetooth + WiFi + WoWLAN + band + scan interval via `set_network_cfg`; microphone + assistant + profile + speech response via `set_voice_control_cfg`).

#### Scenario: Core rejects a value
- **WHEN** the user moves the display brightness slider and the core answers with an error
- **THEN** the slider snaps back to the previous value and "Error setting display config: …" is shown

#### Scenario: Unchanged toggle
- **WHEN** a toggle is set to the value it already has
- **THEN** no request is sent

### Requirement: Display settings
The Display page SHALL offer "Auto brightness" (toggle, "Automatically adjust the display brightness based on ambient lighting conditions.") and "Display brightness" (slider 5–100, step 1), plus "Button backlight" (auto toggle, "When on, button backlight will automatically turn on in a dark room.") and "Button backlight brightness" (slider 0–100, step 1). On Remote Two the UI SHALL additionally dim its own rendering by a black overlay with opacity (100 − brightness)/100 whenever the display brightness changes.

#### Scenario: Minimum display brightness
- **WHEN** the user drags the display brightness slider fully left
- **THEN** the value sent is 5, never lower

#### Scenario: Remote Two dimming
- **WHEN** the display brightness is set to 40 on Remote Two
- **THEN** the UI is drawn under a 60 % black overlay

### Requirement: Sound and haptic settings
The Sound page SHALL offer "Sound effects" (toggle), "Sound effects volume" (slider 0–100) and "Haptic feedback" (toggle). Sound effect playback SHALL follow the enabled flag and volume as soon as the core confirms them, and the initial values at startup come from the core configuration.

#### Scenario: Volume changed
- **WHEN** the core confirms a new sound volume
- **THEN** subsequent UI sound effects play at that volume

### Requirement: Power saving settings
The Power page SHALL offer "Wakeup sensitivity" (slider 0–3: Off, low, medium, high; "Amount of movement needed to wake up the remote."), "Display off timeout" (slider 10–60 s, shown as "Ns") and "Sleep timeout" (slider 10–300 s, shown as minutes and seconds, high label "5 minutes"). "Keep WiFi connected in standby" (WoWLAN toggle) and the wakeup sensitivity row SHALL be shown always on Remote Two, and on Remote 3 only when the WiFi hardware reports wake-on-WLAN support (`UC_WOWLAN=true`).

#### Scenario: Sleep timeout
- **WHEN** the user sets the sleep slider to 120
- **THEN** `set_power_saving_cfg` is sent with `standby_sec` 120 and the current display-off and sensitivity values

#### Scenario: Remote 3 without WoWLAN
- **WHEN** the Power page opens on a Remote 3 without wake-on-WLAN support
- **THEN** the WoWLAN and wakeup sensitivity rows are hidden and the first focus goes to the retry slider

### Requirement: Command retry window after wakeup
The Power page SHALL offer "Retry commands after wakeup" as a slider 0–10 s (0 shown as "Disabled", default 2 s) with the text "Retry commands within N second(s) after wakeup.". The value is a local UI setting (not sent to the core) and takes effect immediately for entity commands and activity readiness waits.

#### Scenario: Disabled
- **WHEN** the slider is set to 0
- **THEN** a command sent while the remote is waking up is not retried

### Requirement: Network settings
The WiFi page SHALL offer "Bluetooth" and "WiFi" toggles, "Active WiFi scanning" (toggle; off sends interval 0, on sends 10 s) with a scan interval slider 10–60 s step 5 shown while scanning is on ("Actively scan for nearby WiFi networks in the configured interval: N seconds"), and a "WiFi band" selector (Auto / 2.4 GHz / 5 GHz, values `auto` / `b` / `a`) on Remote 3 and desktop only. Known networks, the network list and "Delete all networks" are shown only while WiFi is enabled. The Bluetooth address from the network configuration SHALL be shown read-only on the About page.

#### Scenario: Scanning turned on
- **WHEN** the user enables active scanning
- **THEN** `set_network_cfg` is sent with `scan_interval_sec` 10 and the slider appears

#### Scenario: Band on Remote Two
- **WHEN** the WiFi page opens on Remote Two
- **THEN** no band selector is shown

### Requirement: Software update settings
The Software update page SHALL offer "Check for updates" (toggle, "Automatically check for updates.") and, only while it is on, "Auto update" (toggle) with the text "Automatically update the remote when new software is available. Updates are installed between <ota_window_start> and <ota_window_end>". When the core's update channel is `TESTING` a read-only "Beta updates" / "Enabled" row SHALL be shown. OTA window values that the core sends empty SHALL keep the previous value; the channel defaults to `DEFAULT` until the core reports it.

#### Scenario: Auto update hidden
- **WHEN** "Check for updates" is switched off
- **THEN** the "Auto update" row disappears

### Requirement: Voice control settings
The Voice page SHALL offer "Microphone" (toggle, "Disabling the microphone will completely turn it off.  You won’t be able to use voice assistants."). While the microphone is on it SHALL show the selected voice assistant name and "Profile: <name>" (or "None selected" / "No profile selected") read-only with the hint "Use the Web Configurator to edit voice assistants.", and, when an assistant is selected, a "Speech response" toggle ("Play speech response from Voice Assistant when supported.").

#### Scenario: Microphone off
- **WHEN** the microphone toggle is switched off
- **THEN** the assistant rows are hidden

### Requirement: Web configurator access and PIN
Enabling the web configurator SHALL generate a random 4-digit PIN (zero-padded, 0000–9999) and send `set_api_access` with enabled=true and that PIN; disabling sends enabled=false. "Generate new PIN" SHALL send a new PIN and also enable access. The PIN SHALL be displayed only after the core confirms; before the first generation it reads "••••". The enabled state SHALL be read from the core at every connection. The configurator address shown is the device host name, with the WiFi IP address shown on tap where available.

#### Scenario: Enable configurator
- **WHEN** the user switches the web configurator on
- **THEN** a 4-digit PIN is shown after the core confirms and the QR code / address rows become visible

#### Scenario: Core rejects
- **WHEN** `set_api_access` fails
- **THEN** the switch stays off and "Error enabling the web configurator: …" is shown

### Requirement: Administrator PIN
Setting the administrator PIN SHALL send `set_profile_cfg` with `admin_pin`; the PIN page reports success or failure to its caller and shows "Error white setting admin pin: <message>" on failure.

#### Scenario: PIN set
- **WHEN** the entered PIN is confirmed by the core
- **THEN** the page reports success and closes

### Requirement: Device name
Renaming the device SHALL send `set_device_cfg` with the new name and apply it on success; failure shows "Error setting device name: …".

#### Scenario: Rename
- **WHEN** the user confirms a new device name
- **THEN** the new name is used after the core confirms

### Requirement: Local UI preferences
The following settings SHALL be stored locally in `config.ini` under `UC_CONFIG_HOME`, apply immediately without a core round-trip and default as listed: "Inverted button behaviour" (false; "Inverts button functions on the main screen: short press to open the control screen, long press to quick toggle."), "Show battery percentage" (false), "Show battery indicator everywhere" (false), "Activities on pages" (true), "Open activities started with the API" (false), "Zoom media image" (false), "Coverflow in media browser" (false), "Touch slider" enabled (true), touch slider gains volume 0.4, brightness 1.2, cover position 1.2, seek 1.0 (each 0.1–2.0, step 0.1), retry window 2 s.

#### Scenario: Fresh install
- **WHEN** no `config.ini` exists
- **THEN** the UI page shows the defaults above

#### Scenario: Touch slider disabled
- **WHEN** "Touch slider" is off
- **THEN** the gain sliders are disabled and swiping the hardware slider does nothing anywhere

### Requirement: Touch slider gain test area
The Touch Slider page SHALL show a test area labelled "Test – <setting>" for the gain slider last focused or moved, and swiping the hardware slider SHALL move the test value with that gain ("Slide the hardware slider to test the highlighted setting.").

#### Scenario: Selecting a gain
- **WHEN** the user focuses the "Brightness" gain slider
- **THEN** the test area switches to Brightness with its icon

### Requirement: Current profile follows the core
The current profile id SHALL be taken from `get_active_profile` at each connection; a `switch_profile` confirmed by the core SHALL update it and reload the profile and its pages.

#### Scenario: Profile switched
- **WHEN** `switch_profile` succeeds
- **THEN** the profile, pages and groups of the new profile are loaded
