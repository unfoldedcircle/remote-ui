## MODIFIED Requirements

### Requirement: Hiding the keyboard
The keyboard SHALL be hidden when the hide-keyboard key is tapped, when the focused text field loses the focus, when a Button, Switch, Checkbox, dropdown or keypad-activatable row receives the focus while the keyboard is up, when a text-entry dialog is cancelled, submitted or closed, when a selection list popup or an entity list is closed, when the onboarding starts, and when the `OPEN_CASE` warning screen appears (see `core-connection`). Whenever the keyboard is hidden, every text field and search field SHALL lose the focus, except a text field whose keyboard follows the keypad focus (see "A field that follows the keypad focus keeps it when the keyboard goes down"). When a new profile or a profile rename cannot be sent to the core at all, the keyboard SHALL come back after the failure animation; a rejection by the core SHALL NOT bring it back.

#### Scenario: Hide key
- **WHEN** the user taps the hide-keyboard key while the name field of a rename dialog has the focus
- **THEN** the keyboard slides down and the text field loses its focus outline

#### Scenario: Focus moves to a button
- **WHEN** DPAD_DOWN moves the focus from the name field of a rename dialog to its Cancel button
- **THEN** the keyboard is hidden so the buttons are not covered

#### Scenario: Dialog closed
- **WHEN** the "Name your page" dialog is closed with Cancel, BACK or HOME
- **THEN** the dialog and the keyboard are hidden together

#### Scenario: Profile name cannot be sent
- **WHEN** "Add profile" is submitted while the request cannot be sent to the core
- **THEN** the field shows "There was an error. Try again", the failure animation plays and the keyboard is shown again afterwards

## ADDED Requirements

### Requirement: A field that follows the keypad focus keeps it when the keyboard goes down
A text field whose keyboard follows the keypad focus, such as the integration and dock setup fields,
the Wi-Fi network name and password, the new group name and the web view's address field, SHALL keep
the focus when the keyboard is hidden while the field has it, by the hide-keyboard key or by its form:
the keyboard SHALL stay down and the field SHALL stay selected. A tap on the field, or a keypad move
away and back, SHALL bring the keyboard back.

#### Scenario: Hide key on the Wi-Fi password form
- **WHEN** the Wi-Fi password field has the focus with the keyboard up and the user taps the
  hide-keyboard key
- **THEN** the keyboard slides down and stays down, and the field keeps its focus outline

#### Scenario: Keyboard back with a tap
- **WHEN** the keyboard was hidden that way and the user taps the field
- **THEN** the keyboard comes up again for the field

#### Scenario: Load on the web view's developer page
- **WHEN** the user presses Load while the keyboard is up for the address field
- **THEN** the keyboard goes down and the address field stays selected
