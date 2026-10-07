## MODIFIED Requirements

### Requirement: Adding a profile
The "+" footer SHALL open a menu "Add a new profile" with the entries "Normal" and "Restricted". Both open the "Profile name" dialog (placeholder "John", Cancel and Add buttons, on-screen keyboard shown). The name is sent with `add_profile`; a restricted profile is created with the restricted flag. The remote SHALL switch to the new profile exactly once, driven by the profile NEW event of the core, so that the remote and the core always agree on the active profile.

#### Scenario: Empty name
- **WHEN** Add is triggered with an empty name outside onboarding
- **THEN** the field shows an error and nothing is sent
- **AND** during onboarding the placeholder text is used as the name instead

#### Scenario: Profile created
- **WHEN** the core confirms `add_profile`
- **THEN** the dialog and the hosting overlay close, the list is updated, and the profile NEW event of the core makes the new profile the current profile

#### Scenario: Exactly one switch per created profile
- **WHEN** a profile is created on the remote
- **THEN** exactly one `switch_profile` request is sent for it, and no profile is made current without telling the core

#### Scenario: Name already exists
- **WHEN** the core answers with code 422
- **THEN** outside onboarding the warning notification "Profile already exists" is shown; during onboarding an actionable warning "Profile already exists" offers "Choose existing", which returns to the profile list

#### Scenario: Other error
- **WHEN** the core answers with any other error
- **THEN** a warning notification "Error adding profile: <message>" is shown and the form is reset

#### Scenario: Cancel
- **WHEN** Cancel is tapped or BACK or HOME is pressed in the dialog
- **THEN** the dialog closes, the field is cleared and the keyboard hides; the profile list behind it stays open

#### Scenario: First profile
- **WHEN** the "Profile name" dialog is shown full screen because no profile exists yet
- **THEN** it has no Cancel button and no close icon, and BACK or HOME only clear the field and hide the keyboard: the dialog stays, as there is nothing to go back to

### Requirement: Profile page header rows
The profile page (opened by tapping the profile icon in the status bar) SHALL show above the settings menu: "Your current profile" with the profile icon, name and an arrow (tap opens the profile list); the web configurator switch labelled "Web configurator enabled" / "Web configurator disabled"; when enabled and the host name is known, the address `http://<IP address>/configurator`, which a tap switches to `http://<host name>/configurator` and back while an IP address is known (the IP address SHALL stay available: a `.local` host name does not resolve in every network); the 4-digit web configurator PIN in four boxes with a regenerate button; and a 60 px QR code of the address. A restricted profile SHALL instead see a lock with "Restricted" and none of the web configurator rows.

#### Scenario: Address row tapped
- **WHEN** the address row is tapped or activated and the WiFi IP address is known
- **THEN** the address toggles between the IP address and the hostname

#### Scenario: QR code tapped
- **WHEN** the QR code is tapped or activated
- **THEN** it is enlarged to full screen with "Scan to open the Web Configurator" and "Press BACK to close"; a tap, DPAD_MIDDLE, BACK or HOME return to the page

#### Scenario: Header rows by d-pad
- **WHEN** DPAD_UP is pressed on the first menu entry
- **THEN** the selection enters the header at its last visible row; DPAD_UP / DOWN walk the rows, DPAD_DOWN past the last row returns to the menu, DPAD_MIDDLE activates the row with a haptic click

### Requirement: Web configurator access and PIN
Enabling the web configurator SHALL generate a random 4-digit PIN (0000–9999, zero-padded) and send `set_api_access` with enabled=true and the PIN; disabling sends enabled=false. The regenerate button generates a new PIN and enables access. The QR code encodes `http://<hostname>/configurator` as a 200 px PNG.

#### Scenario: Access change confirmed
- **WHEN** the core confirms `set_api_access`
- **THEN** the switch state and the PIN boxes update to the new values

#### Scenario: Access change rejected
- **WHEN** the core rejects `set_api_access`
- **THEN** a warning notification "Error enabling the web configurator: <message>" is shown and the switch keeps its old state

#### Scenario: PIN unknown after connect
- **WHEN** the web configurator state is read from the core on connect
- **THEN** only the enabled flag is applied; the PIN boxes show "••••" until a PIN is generated in this session

#### Scenario: Web configurator screen from the pull-down menu
- **WHEN** the globe icon of the pull-down menu is tapped
- **THEN** a "Web Configurator" screen shows the same switch, address, PIN and regenerate rows and a large QR code; BACK / HOME close it
- **AND** with the keypad the selection starts on the switch; DPAD_UP / DPAD_DOWN walk the switch, the address and the regenerate button (each shown only while the configurator is enabled), DPAD_MIDDLE activates the selected row with a haptic click
