## MODIFIED Requirements

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
