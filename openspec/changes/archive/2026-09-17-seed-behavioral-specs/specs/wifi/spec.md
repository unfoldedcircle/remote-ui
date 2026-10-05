## ADDED Requirements

### Requirement: WiFi status
On every connection to the core the UI SHALL request the WiFi status and the saved networks. From the status the UI SHALL always take the MAC address; when the supplicant state is `COMPLETED` it SHALL mark the remote connected and record the current network (id, SSID, native SSID, RSSI, key management, ciphers, frequency; security derived from the key management string with "-" replaced by "_", `OPEN` when empty) and the IP address; any other state SHALL mark the remote disconnected. Until the first status arrives the remote is assumed connected.

#### Scenario: Connected status
- **WHEN** the status reports COMPLETED with SSID "Home", RSSI -55 and frequency 5180
- **THEN** the current network is "Home" with EXCELLENT signal on 5 GHz and the remote is connected

#### Scenario: Disconnected status
- **WHEN** the status reports DISCONNECTED or ERROR
- **THEN** the remote is marked disconnected and the last current network is kept

### Requirement: WiFi events
The UI SHALL react to the core's WiFi events: `CONNECTED` marks the remote connected and refreshes the status; `DISCONNECTED` marks it disconnected; `SCAN_STARTED`/`SCAN_COMPLETED` set the scan-active flag; `SCAN_FAILED` clears it and reports a failed scan; `NETWORK_NOT_FOUND` reports a missing network; `WRONG_KEY` reports a wrong key and shows the notification "Wrong network key"; `NETWORK_ADDED` and `NETWORK_REMOVED` are ignored.

#### Scenario: Wrong password
- **WHEN** the core sends WRONG_KEY
- **THEN** the notification "Wrong network key" is shown

#### Scenario: Connected event
- **WHEN** the core sends CONNECTED
- **THEN** the remote is marked connected, pending join dialogs report success and the status is re-read

### Requirement: Signal strength classes
The UI SHALL map an RSSI to a signal class: 0 is NONE; at or above -60 dBm EXCELLENT; at or above -68 GOOD; at or above -76 OK; at or above -84 WEAK; below that NONE. Lists SHALL show one bar for WEAK, two for OK and GOOD, three for EXCELLENT and no bars for NONE.

#### Scenario: Weak network
- **WHEN** a scan reports -80 dBm
- **THEN** the network is shown with one bar

### Requirement: Status bar WiFi indicator
The status bar SHALL show a WiFi icon only when the remote is not connected or the current network's signal class is NONE; when not connected the icon SHALL carry a red diagonal strike.

#### Scenario: Connection lost
- **WHEN** the remote becomes disconnected
- **THEN** a struck-through WiFi icon appears in the status bar

#### Scenario: Healthy connection
- **WHEN** the remote is connected with a signal above NONE
- **THEN** no WiFi icon is shown

### Requirement: Scanning while a network page is open
When the WiFi settings page (or the onboarding WiFi page) opens, the UI SHALL read the status, request the saved networks after 500 ms, start a scan after 1 s and poll the scan result every 2 s; once the scan is no longer active it SHALL wait 10 s, re-read the status, start a new scan and resume polling. When the page loses input control (another screen takes over) it SHALL stop the scan, stop the timers and clear the scan list. The dock setup WiFi step SHALL start a scan when entered, poll the result immediately and then every 10 s, and stop the scan when left. A spinner SHALL be shown next to "Other Networks" while a scan is active.

#### Scenario: Settings page opened
- **WHEN** the user opens "Wifi & Bluetooth"
- **THEN** a scan starts within 1 s and the list refreshes every 2 s

#### Scenario: Leaving the page
- **WHEN** the user leaves the page
- **THEN** the scan is stopped and the scan list is emptied

### Requirement: Scan result list rules
The UI SHALL identify a network by its native SSID (hex), falling back to the hex of the UTF-8 friendly name. Access points with an empty SSID SHALL be skipped; of several access points broadcasting the same network only the strongest SHALL be kept. Networks already known (saved) SHALL be excluded from "Other Networks". Existing entries SHALL be updated in place and the list SHALL only be announced as changed when a network appeared or disappeared, so the keypad selection survives a scan. An empty scan result SHALL not clear the list. Lists SHALL be sorted by signal class, strongest first, then by name case-insensitively. A network the core could not classify SHALL be treated as open when it reports no authentication and as WPA2 (secured) otherwise. The list SHALL show the name, band ("2.4 GHz" below 5000 MHz, else "5 GHz"), signal bars and a lock for secured networks, and "No networks found" when empty.

#### Scenario: Two access points
- **WHEN** a scan reports "Home" at -70 and "Home" at -50 dBm
- **THEN** one "Home" entry with EXCELLENT signal is listed

#### Scenario: Scan repeats
- **WHEN** a later scan reports the same networks with different signal levels
- **THEN** the entries update in place and the selected entry stays selected

#### Scenario: Network with non-UTF-8 name
- **WHEN** two networks share the same friendly name but different native SSIDs
- **THEN** both are listed separately and joining one configures exactly that one

### Requirement: Known networks
The UI SHALL list the saved networks under "Known Networks" with their measured signal, "Enabled"/"Disabled" state (disabled when the core state is DISABLED), and for the current network its band. The current network SHALL carry a green marker when connected and a red cross when the remote is disconnected. The list SHALL be updated in place after every saved-network refresh. Known networks SHALL be hidden while WiFi is disabled.

#### Scenario: Current network shown
- **WHEN** the remote is connected to a saved network
- **THEN** that entry shows a green marker and "5 GHz - Enabled" (or 2.4 GHz)

### Requirement: Joining a network from the scan list
Tapping a secured network SHALL open the password dialog "Enter WiFi password for\n<SSID>" with a masked field (placeholder "Super secret", characters revealed for 1 s), Cancel and Join; an empty password SHALL be rejected with the field's error state; Return joins. The join SHALL send an add-network request with SSID, native SSID and password and let the core choose the security type (`AUTO`), then enable the new network and refresh the saved networks after 500 ms. Tapping an open network SHALL show "Join WiFi network?" with the network name and signal, Cancel and Join, and join with security `OPEN`. If the remote is currently disconnected the join SHALL show the loading screen (input blocked, closed by the next connected/failed result or after 180 s). A failed add SHALL show "Error adding network: <message>" and report the join as failed.

#### Scenario: Secured network
- **WHEN** the user taps a locked network, enters a password and presses Join
- **THEN** the network is added with AUTO security, enabled, and the saved list is refreshed

#### Scenario: Open network
- **WHEN** the user confirms "Join WiFi network?" for an unlocked network
- **THEN** the network is added with OPEN security and no password

#### Scenario: Empty password
- **WHEN** the user presses Join with an empty password
- **THEN** the field shows an error and the dialog stays open

### Requirement: Joining another network by name
"Join other" SHALL open a three-step dialog: "Enter SSID" with a required name field and a "Hidden network" checkbox; "Choose WiFi security for\n<SSID>" with the options None, Auto (preselected), "WPA/WPA2 Personal", "WPA2/WPA3 Personal" and, on Remote 3 and desktop only, "WPA3 Personal"; and "Enter WiFi password for\n<SSID>" with a required password. Selecting None SHALL join immediately without a password. The join SHALL send SSID, password, the chosen security and the hidden flag. In the dock setup variant only None and Auto SHALL be offered and the hidden option SHALL be absent; the result is handed to the dock configuration instead of joining. Every step SHALL be walkable with the d-pad, Return submits a field, OK selects an option.

#### Scenario: Hidden WPA3 network on Remote 3
- **WHEN** the user enters a name, ticks "Hidden network", picks "WPA3 Personal" and enters a password
- **THEN** an add-network request with security WPA3_SAE and hidden true is sent

#### Scenario: Remote Two security options
- **WHEN** the dialog is opened on `UCR2`
- **THEN** "WPA3 Personal" is not offered

#### Scenario: No option selected
- **WHEN** the user clears the selection and presses Next
- **THEN** the notification "Select a security option" / "Please select a security option" is shown

### Requirement: Known network actions
Tapping the current known network SHALL open an info sheet with the SSID, connected/disconnected mark, band, "MAC address", "IP address", "Key management", a "Disconnect" (or "Connect") button, "Delete" and "Close". Tapping another known network SHALL open a menu with "Join and disable others" (select command), "Enable"/"Disable" and "Delete". Delete SHALL ask "Remove WiFi network" / "Are you sure you want to remove the network %1?" with "Remove", then send the delete request, remove the entry on success and refresh the saved networks after 1.5 s, or show the core's message on failure. "Delete all networks" SHALL ask "Are you sure you want to delete all WiFi networks?" with "Delete all". Joining a known network SHALL show a spinner on its entry until its id changes or 10 s have passed. Every action SHALL be followed by a saved-network refresh after 500 ms.

#### Scenario: Join and disable others
- **WHEN** the user picks "Join and disable others" on a known network
- **THEN** the select command is sent for that network id and a spinner appears on the entry

#### Scenario: Delete unknown identifier
- **WHEN** a delete is requested for a network that is no longer in the known list
- **THEN** the notification "Failed to delete network. Wifi network does not exist." is shown

### Requirement: Enabling WiFi and Bluetooth
The "Wifi & Bluetooth" page SHALL offer "Bluetooth" and "WiFi" switches. Each change SHALL send the complete network configuration (bluetooth, wifi, WoWLAN, band, scan interval) and adopt the value only after the core confirmed it; failures SHALL show "Error setting Bluetooth: <message>" or "Error setting WiFi: <message>". The network lists and "Delete all networks" SHALL be hidden while WiFi is disabled.

#### Scenario: WiFi disabled
- **WHEN** the user turns the WiFi switch off and the core confirms
- **THEN** the switch shows off and the network lists disappear

### Requirement: Active scanning setting
"Active WiFi scanning" SHALL be on when the scan interval is not 0; turning it on SHALL set the interval to 10 s and turning it off SHALL set 0. While on, a slider from 10 to 60 s in steps of 5 with the text "Actively scan for nearby WiFi networks in the configured interval: %1 seconds" SHALL be shown. Failures SHALL show "Error setting Wifi scan interval: <message>".

#### Scenario: Enable active scanning
- **WHEN** the user turns the switch on
- **THEN** a network configuration with scan interval 10 is sent

### Requirement: WiFi band selection
On Remote 3 and desktop the page SHALL offer "WiFi band" with the choices Auto, 2.4 GHz and 5 GHz (core values `auto`, `b`, `a`) in a popup list; the row SHALL be absent on Remote Two. The choice SHALL be sent as part of the network configuration; failures SHALL show "Error setting Wifi band: <message>". No reboot hint is shown.

#### Scenario: Select 5 GHz
- **WHEN** the user picks "5 GHz"
- **THEN** the network configuration is sent with band `a` and the row shows "5 GHz" once confirmed

### Requirement: Keep WiFi connected in standby
The Power Saving page SHALL offer "Keep WiFi connected in standby" with the text "Keeps WiFi always connected, even when the device is sleeping. Allows for faster reconnect after wakeup. Please note that enabling this feature slightly decreases battery life." The row SHALL be visible on Remote Two always and on other models only when the environment variable `UC_WOWLAN` is `true`. Failures SHALL show "Error setting Wowlan: <message>".

#### Scenario: Remote 3 without flag
- **WHEN** the model is `UCR3` and `UC_WOWLAN` is unset
- **THEN** the row is not shown

### Requirement: Onboarding WiFi step
The onboarding WiFi page SHALL show "Select your WiFi network", the "Wi-Fi address" (MAC), the scan list with "Join other" and a "Skip" button. A join SHALL start a 30 s timeout; a `WRONG_KEY` or `NETWORK_NOT_FOUND` during the attempt or the timeout SHALL count as failure, which deletes all saved networks, shows "Failed to connect" with the explanation and the buttons "Set up later" and "Try again", and restarts scanning. A `DISCONNECTED` event alone SHALL not count as failure. A successful connection SHALL stop scanning and advance to the next step. The SSID and password of the last join SHALL be remembered for the dock setup during onboarding.

#### Scenario: Wrong password during onboarding
- **WHEN** the core reports WRONG_KEY within 30 s of the join
- **THEN** all saved networks are deleted and "Failed to connect" is shown

#### Scenario: Slow join
- **WHEN** no connected event arrives within 30 s
- **THEN** the join is treated as failed

### Requirement: Dock setup WiFi step
When a dock needs WiFi credentials, the dock setup SHALL offer "Select WiFi network" with the scan list; a secured network asks for the password, an open network is taken as is, and "Join other" offers only None/Auto. During onboarding the credentials of the network the remote just joined SHALL be prefilled. The selected SSID and password are passed to the dock and never joined by the remote.

#### Scenario: Dock during onboarding
- **WHEN** the dock setup runs right after the remote joined "Home"
- **THEN** "Home" and its password are prefilled and marked as set
