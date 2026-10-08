## MODIFIED Requirements

### Requirement: Desktop keyboard as keypad
When the UI runs on a desktop (any model), key events from the computer keyboard SHALL be handled like physical buttons whenever their key code belongs to the button map: the arrow keys act as DPAD_UP / DPAD_DOWN / DPAD_LEFT / DPAD_RIGHT, Return as DPAD_MIDDLE, Home as HOME, F3 as VOICE and F4 as MENU. On the desktop model `DEV`, Escape SHALL act as BACK: every Escape key press and release SHALL be replaced by the key event of the BACK button, with the same press or release and auto-repeat flag, so that the button-navigation handlers and the focused control receive BACK exactly as from a device, and no control SHALL receive the Escape event, including a popup that would close on Escape. Backspace and the numeric keypad Enter SHALL NOT act as any button. A held keyboard key auto-repeats like a held hardware key. Any other key only reaches the focused control (e.g. typing into a text field).

#### Scenario: Arrow keys
- **WHEN** the user presses the Down arrow key on the desktop keyboard
- **THEN** the UI reacts as to DPAD_DOWN and the keypad selection highlights become visible

#### Scenario: Escape
- **WHEN** the user presses Escape on a settings page in `DEV`
- **THEN** the UI reacts as to BACK and the page is left

#### Scenario: Escape on a popup that closes on Escape
- **WHEN** the user presses Escape in `DEV` while a popup is open whose close policy includes closing on Escape
- **THEN** the popup does not close because of Escape; it reacts to BACK as on a device

#### Scenario: Held Escape
- **WHEN** the user holds Escape in `DEV`
- **THEN** the UI reacts as to a held BACK button: auto-repeat presses until the key is released, then one BACK release

#### Scenario: Backspace
- **WHEN** the user presses Backspace on a settings page
- **THEN** the page is not left

#### Scenario: Held arrow key
- **WHEN** the user holds the Down arrow key on a list that supports repeat
- **THEN** the selection keeps moving until the key is released
