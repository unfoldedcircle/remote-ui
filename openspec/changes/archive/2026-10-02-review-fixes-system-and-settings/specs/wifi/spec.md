## MODIFIED Requirements

### Requirement: WiFi status
On every connection to the core the UI SHALL request the WiFi status and the saved networks. From the status the UI SHALL always take the MAC address; when the supplicant state is `COMPLETED` it SHALL mark the remote connected and record the current network (id, SSID, native SSID, RSSI, key management, ciphers, frequency, security) and the IP address; any other state SHALL mark the remote disconnected. Until the first status arrives the remote is assumed connected. The key management SHALL be kept and shown exactly as the status reports it. The security type of the current network SHALL be derived from the key management string, ignoring case: empty or `NONE` is open; anything containing `SAE` (e.g. `SAE`, `FT-SAE`) is WPA3; anything containing `EAP` is WPA2 enterprise when it starts with `WPA2` and WPA enterprise otherwise; other strings starting with `WPA2` are WPA2 personal and other strings starting with `WPA` (e.g. `WPA-PSK`, `WPA-PSK-SHA256`) are WPA personal; any other string SHALL be treated as encrypted (WPA2 personal), never as open and never as an unknown type.

#### Scenario: Connected status
- **WHEN** the status reports COMPLETED with SSID "Home", RSSI -55 and frequency 5180
- **THEN** the current network is "Home" with EXCELLENT signal on 5 GHz and the remote is connected

#### Scenario: Disconnected status
- **WHEN** the status reports DISCONNECTED or ERROR
- **THEN** the remote is marked disconnected and the last current network is kept

#### Scenario: WPA3 network
- **WHEN** the remote is connected to a WPA3 network and the status reports the key management `SAE`
- **THEN** the current network's security is WPA3 and the connection details show "Key management" `SAE`

#### Scenario: Key management shown as reported
- **WHEN** the status reports the key management `WPA2-PSK`
- **THEN** the connection details show `WPA2-PSK`, not `WPA2_PSK`, and the security is WPA2 personal

#### Scenario: Unknown key management
- **WHEN** the status reports a key management string the UI does not know
- **THEN** the current network is treated as encrypted (WPA2 personal), not as open

### Requirement: Scanning while a network page is open
When the WiFi settings page (or the onboarding WiFi page) opens, the UI SHALL read the status, request the saved networks after 500 ms, start a scan after 1 s and poll the scan result every 2 s; once the scan is no longer active it SHALL wait 10 s, re-read the status, start a new scan and resume polling. The scan cycle of the WiFi settings page SHALL keep running while a dialog or sheet is open over the page (network info, join, password); the settings page sends no request to stop the scan and keeps its scan list, and its cycle ends when the page itself is unloaded. The onboarding WiFi page SHALL stop the scan and its timers once the remote has joined a network. The dock setup WiFi step SHALL start a scan when entered, poll the result immediately and then every 10 s, and stop the scan when left. A spinner SHALL be shown next to "Other Networks" while a scan is active.

#### Scenario: Settings page opened
- **WHEN** the user opens "Wifi & Bluetooth"
- **THEN** a scan starts within 1 s and the list refreshes every 2 s

#### Scenario: Leaving the page
- **WHEN** the user opens a dialog over the WiFi settings page, or goes back from the page to the settings menu
- **THEN** no scan stop is sent and the scan list is not emptied; the page's polling ends only when the page is unloaded, e.g. when another settings page is opened in its place or the settings are closed

#### Scenario: Scan ends without networks
- **WHEN** a scan ends and its result contains no access point
- **THEN** the scan is no longer reported as active, the spinner disappears and the cycle starts the next scan 10 s later, while the list keeps the networks it showed

### Requirement: Scan result list rules
The UI SHALL identify a network by its native SSID (hex), falling back to the hex of the UTF-8 friendly name. Access points with an empty SSID SHALL be skipped; of several access points broadcasting the same network only the strongest SHALL be kept. Networks already known (saved) SHALL be excluded from "Other Networks". Existing entries SHALL be updated in place and the list SHALL only be announced as changed when a network appeared or disappeared, so the keypad selection survives a scan. An empty scan result SHALL not clear the list, but its scan state SHALL still be taken over. Forgetting a saved network, or deleting all of them, SHALL announce "Other Networks" as changed as well, so a forgotten network that the last scan found is listed there again right away. Should the list be emptied, it SHALL be announced as empty before its entries are discarded, so no row shows a discarded entry. Lists SHALL be sorted by signal class, strongest first, then by name case-insensitively. A network the core could not classify SHALL be treated as open when it reports no authentication and as WPA2 (secured) otherwise. The list SHALL show the name, band ("2.4 GHz" below 5000 MHz, else "5 GHz"), signal bars and a lock for secured networks, and "No networks found" when empty.

#### Scenario: Two access points
- **WHEN** a scan reports "Home" at -70 and "Home" at -50 dBm
- **THEN** one "Home" entry with EXCELLENT signal is listed

#### Scenario: Scan repeats
- **WHEN** a later scan reports the same networks with different signal levels
- **THEN** the entries update in place and the selected entry stays selected

#### Scenario: Network with non-UTF-8 name
- **WHEN** two networks share the same friendly name but different native SSIDs
- **THEN** both are listed separately and joining one configures exactly that one

#### Scenario: Forgotten network
- **WHEN** the user deletes the saved network "Home" (or all saved networks) and the last scan found "Home"
- **THEN** "Home" appears under "Other Networks" as soon as the core confirmed the deletion, without waiting for a later scan to change the list

#### Scenario: List emptied
- **WHEN** the scan list is emptied while the list view is shown
- **THEN** the view shows the empty list first and no row refers to a discarded network
