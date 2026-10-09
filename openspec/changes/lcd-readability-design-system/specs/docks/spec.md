## MODIFIED Requirements

### Requirement: Docks settings page keypad walk
DPAD_UP / DPAD_DOWN SHALL move the selection through the dock list; DPAD_DOWN past the last dock selects the "Add a new dock" sheet and DPAD_UP from there returns to the list. DPAD_MIDDLE opens the selected dock's details or the sheet. While the list is empty the sheet is selected. When a dock below the selection is removed, the selection stays on the last dock. The Identify / Connect buttons of a card are not reachable by keypad; the same actions are reachable in the dock details.

#### Scenario: Empty dock list
- **WHEN** no dock is configured and the page is opened by keypad
- **THEN** the "Add a new dock" sheet is selected and DPAD_MIDDLE opens it

### Requirement: Dock discovery in the "Add a new dock" sheet
The "Add a new dock" sheet SHALL open on a start screen with the text "Select Discover to search for docks on your network or via Bluetooth. If you would like to wirelessly setup a new dock, make sure it's in close proximity to the remote." and a "Discover" button. When Bluetooth is disabled the screen SHALL additionally show the red hint "Bluetooth is disabled. Discovery limited to network only." with a Bluetooth switch that enables it. Discover SHALL clear the previous results and ask the core to start a discovery over Bluetooth and network for new docks only, with the core's default timeout of 30 s. Closing the sheet SHALL stop the discovery.

#### Scenario: Discovery results
- **WHEN** the core reports a discovered dock that is not yet configured
- **THEN** it is listed with an ethernet icon and its friendly name (network discovery) or a Bluetooth icon and its id (Bluetooth discovery), plus its address; a dock reported twice is listed once
- **AND** the header shows "Discovering" with a spinner while the discovery runs and "N dock(s) found" once it stops; tapping the header starts a new discovery

#### Scenario: Discovery cannot be started or stopped
- **WHEN** the core rejects the start or stop request
- **THEN** an actionable warning "Failed to start (stop) dock discovery – There was an error starting (stopping) dock discovery: <message>" with a "Try again" action is shown

#### Scenario: Keypad walk of the discovery
- **WHEN** the start screen is shown
- **THEN** the selection starts on its first control (the Bluetooth switch if shown, otherwise Discover); DPAD_UP / DPAD_DOWN move between them and DPAD_MIDDLE activates the selected control
- **AND** once results are listed the first result is selected automatically; DPAD_UP / DPAD_DOWN move over the results and DPAD_MIDDLE starts the setup of the selected dock; BACK / HOME stop the discovery

### Requirement: Dock details popup
Tapping a dock card (or DPAD_MIDDLE on the selected card) SHALL open a full-screen details popup that first refreshes the dock from the core, showing the dock image and name ("Select to edit the name"), the "Something is wrong" line in state `ERROR`, Identify (states `ACTIVE` / `IDLE`) or Connect (state `ERROR`), the fields State, Connection type, Service name, Custom IP or URL ("Not set" when empty), Firmware version ("N/A" when unknown), a "Led brightness" slider from 0 to 100 in steps of 1, the rows "Change password" and "Factory reset" ("Change WiFi settings" is hidden until it is implemented), and a "Delete dock" drawer at the bottom. Name editing, the slider and the two rows are enabled only in states `ACTIVE` and `IDLE` and shown at 30 % opacity otherwise.

#### Scenario: Refresh fails
- **WHEN** the core cannot deliver the dock's current data
- **THEN** the notification "There was an error while getting the latest dock data" is shown and the cached data is used

#### Scenario: Keypad walk of the details
- **WHEN** the popup opens by keypad
- **THEN** the selection starts on the name row (or on Connect / the delete opener when the name cannot be edited)
- **AND** DPAD_DOWN walks name → Identify / Connect → LED slider → Change password → Factory reset → Delete, skipping invisible or disabled rows; the focused row is scrolled into view; BACK / HOME close the popup

#### Scenario: LED brightness
- **WHEN** the user drags the slider and releases it, or presses DPAD_LEFT / DPAD_RIGHT on it
- **THEN** the new brightness is sent to the dock (each keypad step immediately); a rejected command shows the core's message

#### Scenario: Change WiFi settings
- **WHEN** the details popup is shown
- **THEN** it has no "Change WiFi settings" row: changing the dock's WiFi is not implemented, and a row that does nothing would be a dead end for the keypad
