## MODIFIED Requirements

### Requirement: Start step
The Start step SHALL show a greeting that cycles through "Hallo", "Hoi", "Szia", "Hej", "Hei", "Hello" every 5 s with a 400 ms fade, and the hint "Press OK or touch the screen to begin". Tapping anywhere or DPAD_MIDDLE SHALL advance to the Terms step.

#### Scenario: BACK on Start
- **WHEN** BACK is pressed on the Start step
- **THEN** nothing happens

### Requirement: Terms & conditions step
The Terms step SHALL show the text "By using Unfolded Circle products you agree to the Terms & conditions. You can read them on unfoldedcircle.com/legal or by scanning this QR code. Select the QR code to read them on the screen.", a 300 px QR code linking to https://unfoldedcircle.com/legal, and the buttons Cancel and Agree. Agree SHALL advance; Cancel SHALL go back to Start.

#### Scenario: Keypad on the Terms step
- **WHEN** the step opens
- **THEN** the selection starts on Cancel; DPAD_RIGHT reaches Agree, DPAD_UP reaches the QR code, DPAD_DOWN from the QR code returns to Cancel

#### Scenario: Reading the terms on screen
- **WHEN** the QR code is tapped or DPAD_MIDDLE is pressed on it
- **THEN** a full-screen "Terms & conditions" popup shows the terms text, scrollable with DPAD_UP / DPAD_DOWN
- **AND** the X icon, BACK or HOME closes the popup and returns to the step

### Requirement: Finish step
The Finish step SHALL show "You're all set – You can add integrations or change configuration via the Web configurator." and, for an unrestricted profile, a box with the web configurator switch ("Web configurator enabled" / "disabled"), the configurator address (the host name first; tapping it or DPAD_MIDDLE on it toggles to the IP address and back), the 4-digit configurator PIN with a button that generates a new PIN, and a QR code of the address, all shown only while the configurator is enabled. The page SHALL scroll so Done is reachable in every language. Done SHALL end the onboarding and show the "Useful tips" help overlay.

#### Scenario: Keypad on the Finish step
- **WHEN** the step is shown
- **THEN** the selection starts on the web configurator switch (or on Done for a restricted profile); DPAD_DOWN reaches the address, the new-PIN button and then Done; the focused control is scrolled into view; BACK does nothing

#### Scenario: Tips after Done
- **WHEN** the help overlay is shown right after the onboarding
- **THEN** DPAD_LEFT / DPAD_RIGHT only move between the tips and do not switch the pages behind them; OK, BACK or HOME closes the tips
