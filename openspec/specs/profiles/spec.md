# profiles Specification

## Purpose

User profiles on the remote: the profile list and switching, restricted profiles and the administrator PIN, adding, renaming, re-iconing and deleting profiles, the web-configurator access rows, and factory reset.

## Requirements

### Requirement: Profile list is loaded from the core
The UI SHALL load the list of profiles (id, name, icon, restricted flag) from the core with `get_profiles` once all entities have been loaded after a connect, and again every time the profile list screen is opened. A profile without an icon SHALL be shown with the default user icon (`uc:user`).

#### Scenario: Profile list request fails
- **WHEN** the `get_profiles` request is answered with an error
- **THEN** the UI retries the request after 2 seconds, indefinitely, without showing an error

#### Scenario: Profile list screen opened
- **WHEN** the profile list is shown
- **THEN** the list is re-requested from the core and rebuilt from the response
- **AND** the row of the current profile is preselected

### Requirement: Current profile is determined by the core
The current profile SHALL be requested from the core with `get_active_profile` on every connect. The profile is then loaded with `get_profile` (name, icon, restricted flag), followed by its groups and its pages. Switching profiles is persisted only through the core (`switch_profile`); the UI keeps no local copy across restarts. When the groups and pages of two profile loads overlap, for example a reconnect during a profile switch, the groups and pages of the most recently requested load SHALL be shown and those of the earlier load dropped (see `pages` and `groups`).

#### Scenario: Active profile loaded
- **WHEN** the core answers `get_active_profile` and `get_profile` successfully
- **THEN** the profile name, icon and restricted state are shown, the groups of the profile are fetched and its pages are loaded

#### Scenario: No active profile
- **WHEN** `get_active_profile` or `get_profile` fails and the UI is not onboarding
- **THEN** any open overlays are closed
- **AND** if the profile list is empty the "Profile name" dialog is shown full screen to create the first profile, otherwise the profile list is shown full screen in "no profile" mode

#### Scenario: No profile mode of the list
- **WHEN** the profile list is shown in "no profile" mode
- **THEN** it has no back arrow and BACK / HOME do not close it; the only way out is selecting or adding a profile

#### Scenario: Reconnect during a profile switch
- **WHEN** the app reconnects to the core while the pages and groups of a profile switch are still loading
- **THEN** the pages and groups of one profile are shown, each once, and never a mix of two profiles

### Requirement: Profile list rows
Each profile row SHALL show the profile icon, the name (single line, elided, font scaled down to a minimum of 30 px), a lock icon when the profile is restricted and a green dot when it is the current profile. The list has the title "Profiles" and a back arrow, and ends with a "+" footer of 150 px height.

#### Scenario: Footer visibility
- **WHEN** the current profile is restricted or the UI is onboarding
- **THEN** the "+" footer is not shown

### Requirement: Switching profiles
Tapping a profile row or pressing DPAD_MIDDLE on the selected row SHALL switch to that profile. When the current profile is not restricted the switch is sent immediately with an empty `admin_pin`. When the current profile is restricted, the administrator PIN prompt is shown first and the switch is sent with the entered PIN. The core validates the PIN; the UI performs no local PIN check.

#### Scenario: Switch succeeds
- **WHEN** the core confirms `switch_profile`
- **THEN** the loading indicator shows success, the new profile becomes the current profile and its pages are loaded
- **AND** the profile list and the overlay that hosted it close

#### Scenario: Switch fails
- **WHEN** the core rejects `switch_profile` (for example wrong PIN)
- **THEN** the loading indicator stops, a warning notification with the core's error message is shown
- **AND** if the PIN prompt is open, the four PIN dots turn red for 2 seconds and the entered digits are cleared so the PIN can be entered again

#### Scenario: Switch while disconnected
- **WHEN** the switch cannot be sent because the core is not connected
- **THEN** the loading indicator shows a failure

### Requirement: Administrator PIN prompt
The PIN prompt SHALL show the text "Please enter the administrator PIN.", four PIN dots, a 3x4 digit keypad (1–9, 0 and backspace) and a Cancel button. The PIN is exactly 4 digits and is submitted automatically when the fourth digit is entered. The prompt opens with no digits and no keypad outline.

#### Scenario: Keypad by d-pad
- **WHEN** DPAD_UP / DOWN / LEFT / RIGHT is pressed the first time
- **THEN** the "1" key is outlined without entering a digit; further presses move the outline within the grid, skipping the empty cell
- **AND** DPAD_MIDDLE enters the outlined digit (the first DPAD_MIDDLE only shows the outline)

#### Scenario: Keypad by touch
- **WHEN** a digit is tapped
- **THEN** the digit is entered, no key is outlined, and a haptic click is played

#### Scenario: Cancel the prompt
- **WHEN** Cancel is tapped or BACK or HOME is pressed
- **THEN** the prompt closes without switching

### Requirement: Setting the administrator PIN
The administrator PIN SHALL be set from the "Administrator PIN" settings page (and during onboarding) by entering a 4-digit PIN on one keypad and repeating it on a second keypad. The PIN is stored by the core via `set_profile_cfg`.

#### Scenario: PINs match
- **WHEN** the second entry equals the first
- **THEN** the PIN is sent to the core; on success the page returns to the settings menu

#### Scenario: PINs differ
- **WHEN** the second entry differs from the first
- **THEN** the warning notification "The pin doesn't match. Try again." is shown, both entries are cleared and the first keypad is shown again

#### Scenario: Core rejects the PIN
- **WHEN** `set_profile_cfg` fails
- **THEN** a warning notification with the error is shown, both entries are cleared and the first keypad is shown again

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

### Requirement: Profile menu (rename, icon, delete)
A long press on a profile row (touch) or a long press on DPAD_MIDDLE on the selected row SHALL open a menu titled with the profile name with the entries "Rename", "Edit icon" and "Delete". When the current profile is restricted the menu SHALL not open and an error haptic is played.

#### Scenario: Rename
- **WHEN** "Rename" is chosen
- **THEN** the "Rename profile" dialog opens prefilled with the current name; an empty name shows a field error; Rename sends `update_profile` with the new name and closes the dialog; BACK / HOME cancel

#### Scenario: Edit icon
- **WHEN** "Edit icon" is chosen and an icon is selected in the icon selector
- **THEN** `update_profile` is sent with the icon

#### Scenario: Update confirmed
- **WHEN** the core confirms `update_profile`
- **THEN** the name, icon and restricted flag in the list are replaced with the values returned by the core, and the current profile's header is updated if it was the one changed

#### Scenario: Update rejected
- **WHEN** the core rejects `update_profile`
- **THEN** an actionable warning "Profile update error" with the core's message is shown

#### Scenario: Delete another profile
- **WHEN** "Delete" is chosen for a profile that is not the current one
- **THEN** `delete_profile` is sent immediately without confirmation and without a PIN; a failure shows a warning notification "Error deleting profile: <message>"

#### Scenario: Delete the current profile
- **WHEN** "Delete" is chosen for the current profile
- **THEN** nothing is sent and an actionable warning "Error" reads "Deleting a current profile is not permitted. Please switch to another profile and try again."

### Requirement: Core-driven profile changes
The UI SHALL apply profile events from the core (`profile_id` without `page_id` / `group_id`) to the profile list whether the list is open or closed. The profile identifier SHALL be taken from the event itself and all other profile data — name, icon, restricted flag, description and page list — from the profile object of `new_state`, for the NEW and the CHANGE event alike. A CHANGE event that carries no profile object SHALL be ignored, because it carries no profile data to apply.

#### Scenario: Profile added by another client
- **WHEN** a NEW profile event arrives
- **THEN** the profile is appended to the list
- **AND** the UI sends `switch_profile` to that profile, so the remote switches to any profile created through the Core-API

#### Scenario: Restricted profile announced by a NEW event
- **WHEN** a NEW profile event announces a profile whose profile object has `restricted` set
- **THEN** the profile is listed with the lock icon and, once it is the current profile, the restricted limits apply immediately, without waiting for the next profile load

#### Scenario: Profile changed
- **WHEN** a CHANGE profile event with a profile object arrives
- **THEN** the name and icon in the list (and in the header if it is the current profile) are replaced when the event carries a non-empty value, and the restricted flag is always replaced

#### Scenario: Change event without a profile object
- **WHEN** a CHANGE profile event arrives that has no profile object, as the core sends it when only the pages of a profile changed
- **THEN** nothing is applied: the name, icon and restricted flag of the profile keep their current values

#### Scenario: Profile deleted
- **WHEN** a DELETE profile event arrives
- **THEN** the profile is removed from the list
- **AND** if it was the current profile, its name, icon and restricted flag are cleared, its pages are dropped from the screen and the profile selection is brought up, exactly as switching to another profile does

#### Scenario: The last profile is deleted
- **WHEN** a DELETE profile event removes the current profile and no profile is left
- **THEN** the "Profile name" dialog is shown full screen so the first profile can be created

### Requirement: D-pad walk of the profile list
The profile list SHALL be walked with the d-pad: DPAD_DOWN moves the selection down and, past the last profile, onto the "+" footer (when shown); DPAD_UP moves it up and from the footer back to the last profile; DPAD_MIDDLE switches to the selected profile or opens the add menu on the footer; a long press on DPAD_MIDDLE opens the profile menu; BACK closes the list; HOME closes the list and the settings overlay hosting it. The selected row is drawn with a dark background and border, the selected footer with a highlight border; both only while key navigation is active.

#### Scenario: Keys while the keyboard is open
- **WHEN** the on-screen keyboard is visible
- **THEN** DPAD_MIDDLE and its long press on the list are ignored

### Requirement: Profile page header rows
The profile page (opened by tapping the profile icon in the status bar) SHALL show above the settings menu: "Your current profile" with the profile icon, name and an arrow (tap opens the profile list); the web configurator switch labelled "Web configurator enabled" / "Web configurator disabled"; when enabled and the host name is known, the address `http://<IP address>/configurator`, which a tap switches to `http://<host name>/configurator` and back while an IP address is known (the IP address SHALL stay available: a `.local` host name does not resolve in every network); the 4-digit web configurator PIN in four boxes with a regenerate button; and a 60 px QR code of the address. A restricted profile SHALL instead see a lock with "Restricted" and none of the web configurator rows.

#### Scenario: Address row tapped
- **WHEN** the address row is tapped or activated and the WiFi IP address is known
- **THEN** the address toggles between the IP address and the hostname

#### Scenario: QR code tapped
- **WHEN** the QR code is tapped or activated
- **THEN** it is enlarged to full screen with "Scan to open the Web Configurator" and "Tap to close"; a tap, BACK or HOME return to the page

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

### Requirement: Restricted profile limits
When the current profile is restricted, the UI SHALL hide the settings entries Software update, Settings, Integrations and Docks (only About remains), hide the web configurator and settings icons of the pull-down menu, hide the profile "+" footer, refuse the page menu with the notification "Profile is restricted", and hide the page selector's edit pencil.

#### Scenario: Restricted user opens settings
- **WHEN** the settings overlay is opened by a restricted profile
- **THEN** the header shows "Restricted" and the menu contains only "About"

### Requirement: Factory reset token
A factory reset SHALL be a two-step operation: pressing "Erase everything" requests a one-time token with `get_factory_reset_token`; the confirmation SHALL be opened only after a non-empty token was received. Confirm sends `factory_reset` with that token; closing the confirmation by any means clears the token. `factory_reset` SHALL never be sent with an empty token.

#### Scenario: Token received
- **WHEN** the core answers `get_factory_reset_token` with a non-empty token
- **THEN** the point-of-no-return confirmation opens

#### Scenario: Token request fails
- **WHEN** `get_factory_reset_token` cannot be sent, is answered with an error, or is answered with an empty token
- **THEN** the warning notification "The factory reset could not be started. Please try again." is shown, the confirmation is not opened and no reset is sent

#### Scenario: Reset rejected
- **WHEN** `factory_reset` fails
- **THEN** a warning notification "Error factory reset: <message>" is shown
