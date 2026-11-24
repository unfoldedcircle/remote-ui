## MODIFIED Requirements

### Requirement: Dock details popup
Tapping a dock card (or DPAD_MIDDLE on the selected card) SHALL open a full-screen details popup that first refreshes the dock from the core, showing the dock image and name ("Select to edit the name"), the "Something is wrong" line in state `ERROR`, Identify (states `ACTIVE` / `IDLE`) or Connect (state `ERROR`), the fields State, Connection type ("—" when unknown), Service name, Custom IP or URL ("Not set" when empty), Firmware version ("—" when unknown), a "Led brightness" slider from 0 to 100 in steps of 1, the rows "Change password" and "Factory reset" ("Change WiFi settings" is hidden until it is implemented), and a "Delete dock" drawer at the bottom. Name editing, the slider and the two rows are enabled only in states `ACTIVE` and `IDLE` and shown at 30 % opacity otherwise.

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
