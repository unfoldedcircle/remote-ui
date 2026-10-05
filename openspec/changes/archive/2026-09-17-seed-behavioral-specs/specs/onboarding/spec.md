## ADDED Requirements

### Requirement: Onboarding runs while the first-run marker exists
The UI SHALL run the onboarding wizard instead of the main screen when the file named by the onboarding path environment variable exists at start-up. While the onboarding runs, the connecting indicator, integration error notifications and the automatic opening of externally started activities SHALL be suppressed, and the "no profile" prompts of the main screen SHALL not appear. Finishing the onboarding SHALL delete the marker file and load the main screen (or the "no page" screen when the profile has no pages).

#### Scenario: First start
- **WHEN** the marker file exists at start-up
- **THEN** the onboarding container replaces the main container and the start-up spinner is stopped

#### Scenario: Interrupted onboarding
- **WHEN** the remote restarts before the Finish step was completed
- **THEN** the onboarding starts again from the Start step; settings already sent to the core (language, country, timezone, admin PIN, remote name, profile, WiFi, docks, integrations) keep their values

### Requirement: Fixed step order with a progress bar
The onboarding SHALL walk the steps Start → Terms → Language → Country → Timezone → Administrator PIN → Remote name → Profile → WiFi → Dock → Integration → Finish in this order, without swiping. A 6 px progress bar at the top SHALL grow with the step index (empty on Start, full on Finish) with a 300 ms animation. Every step SHALL open on its initial control, never on the control it was left on.

#### Scenario: Going forward
- **WHEN** a step calls for the next step
- **THEN** the next step is shown immediately, provided the step's precondition is met (a language, country and timezone must have been selected on their steps, the PIN must have been accepted; all other steps may always advance)

#### Scenario: Going back
- **WHEN** BACK is pressed (or a step's Cancel / back action is used)
- **THEN** the previous step is shown after a 500 ms delay; BACK on Start and on Finish does nothing

### Requirement: Start step
The Start step SHALL show a greeting that cycles through "Hallo", "Hoi", "Szia", "Hej", "Hei", "Hello" every 5 s with a 400 ms fade, and the hint "Tap the screen to begin". Tapping anywhere or DPAD_MIDDLE SHALL advance to the Terms step.

#### Scenario: BACK on Start
- **WHEN** BACK is pressed on the Start step
- **THEN** nothing happens

### Requirement: Terms & conditions step
The Terms step SHALL show the text "By using Unfolded Circle products you agree to the Terms & conditions. You can read them on unfoldedcircle.com/legal or by scanning this QR code. Tap the QR code to show it on the screen.", a 300 px QR code linking to https://unfoldedcircle.com/legal, and the buttons Cancel and Agree. Agree SHALL advance; Cancel SHALL go back to Start.

#### Scenario: Keypad on the Terms step
- **WHEN** the step opens
- **THEN** the selection starts on Cancel; DPAD_RIGHT reaches Agree, DPAD_UP reaches the QR code, DPAD_DOWN from the QR code returns to Cancel

#### Scenario: Reading the terms on screen
- **WHEN** the QR code is tapped or DPAD_MIDDLE is pressed on it
- **THEN** a full-screen "Terms & conditions" popup shows the terms text, scrollable with DPAD_UP / DPAD_DOWN
- **AND** the X icon, BACK or HOME closes the popup and returns to the step

### Requirement: Language step
The Language step SHALL show a searchable "Select language" list of all compiled translations with their native names, rebuilt on every entry with the current language preselected. Selecting a language SHALL send it to the core, mark the step as complete and advance immediately, without waiting for the core's answer; the UI retranslates once the core confirms. Native names are never translated.

#### Scenario: BACK on the language list
- **WHEN** BACK is pressed
- **THEN** the list is hidden and the Terms step is shown

#### Scenario: Return to the step
- **WHEN** the user comes back from the Country step
- **THEN** the list opens again with the newly chosen language preselected

### Requirement: Country step
The Country step SHALL show a searchable "Select country" list requested from the core on every entry, with a "Suggested" section (countries where the chosen language is spoken, the most likely one first and preselected, the others alphabetical) followed by "All countries" alphabetically, names in the UI language with English as fallback. A row is also found by typing its two-letter ISO code. The list always opens at the top. Selecting a country SHALL send it to the core; the step advances only when the core confirms.

#### Scenario: Language confirmation arrives late
- **WHEN** the core confirms the language after the country list was built
- **THEN** the list is rebuilt with names and suggestions in the new language

#### Scenario: Core rejects the country
- **WHEN** the core answers the country change with an error
- **THEN** the step stays on the list

#### Scenario: BACK on the country list
- **WHEN** BACK is pressed
- **THEN** the list is hidden and cleared and the Language step is shown

### Requirement: Timezone step
On entry the Timezone step SHALL look up the timezones of the selected country offline. With exactly one zone it SHALL show a "Confirm timezone" card with the country name and "<city> · GMT±hh:mm" (standard-time offset, no current time), a Confirm button and "Choose another timezone". With several zones it SHALL show a searchable "Select timezone" list of the country's zones (city, zone name, GMT offset, sorted east to west) with the currently configured zone preselected and a last row "All timezones…"; with no zone it SHALL show the world list directly. Selecting a zone or confirming SHALL send it to the core; the step advances only when the core confirms.

#### Scenario: Keypad on the confirm card
- **WHEN** the card is shown
- **THEN** the selection starts on Confirm; DPAD_DOWN reaches "Choose another timezone", DPAD_MIDDLE activates, BACK returns to the Country step

#### Scenario: World list escape hatch
- **WHEN** "All timezones…" or "Choose another timezone" is activated
- **THEN** the list shows every timezone in the world with the configured zone preselected
- **AND** BACK returns to the country list (several zones) or to the confirm card (one zone); BACK on the country list returns to the Country step

### Requirement: Administrator PIN step
The PIN step SHALL show "Administrator PIN – This PIN is the administrator PIN." with a numeric keypad. After 4 digits a second keypad SHALL ask for the same PIN again. Matching PINs SHALL be sent to the core with `set_profile_cfg`; on success the step advances.

#### Scenario: PINs differ
- **WHEN** the confirmation differs from the first entry
- **THEN** both entries are cleared, the first keypad is shown again with an error and the notification "The pin doesn't match. Try again." appears

#### Scenario: Core rejects the PIN
- **WHEN** the core answers with an error
- **THEN** the error is shown as a notification, both entries are cleared and the first keypad is shown again

#### Scenario: Keypad entry
- **WHEN** the d-pad is used
- **THEN** an outline appears on the keypad and moves with DPAD_UP / DOWN / LEFT / RIGHT, DPAD_MIDDLE enters the outlined digit; the outline position carries over to the confirmation keypad; entering by touch shows no outline
- **AND** BACK returns to the Timezone step

### Requirement: Remote name step
The Remote name step SHALL show "Name your remote" with a text field pre-filled with "Remote 3" on Remote 3 and "Remote Two" on Remote Two, the on-screen keyboard open and the field focused, and a Next button. Next or Return with an empty field SHALL mark the field as erroneous; otherwise the name is sent to the core and the step advances when the core confirms.

#### Scenario: Core rejects the name
- **WHEN** the core answers with an error
- **THEN** the error is shown as a notification and the step stays

### Requirement: Profile step
The Profile step SHALL show the profile form with the placeholder "Default" and only an Add button. An empty name SHALL create the profile "Default". On success the UI switches to the new profile and advances 3 s later. If the name already exists, an actionable warning "Profile already exists – The profile name you've entered already exists. Would you like to continue with an existing profile?" with "Choose existing" SHALL be offered; choosing it opens the profile list, and selecting a profile there advances the step. Switching to a profile while the current one is restricted asks for the administrator PIN.

#### Scenario: Profile list closed without a selection
- **WHEN** the profile list is hidden without a profile having been selected
- **THEN** the profile form is shown again

#### Scenario: Adding fails for another reason
- **WHEN** the core rejects the profile with a code other than 422
- **THEN** the failure animation is shown and the form is reset without a notification

### Requirement: WiFi step
The WiFi step SHALL show "Select your WiFi network", the remote's Wi-Fi address, the list of scanned networks (a scan is started 1 s after entry and repeated every 10 s after each scan ends), a "Join other" footer and a Skip button. Selecting an encrypted network asks for its password; the join SHALL be treated as failed only on a wrong password, a network that cannot be found, or after 30 s without a result. On success the step advances after the success animation and scanning stops.

#### Scenario: Join failed
- **WHEN** the join fails
- **THEN** every saved network is deleted, scanning restarts and a "Failed to connect" screen explains that WiFi can be set up later in Settings and that dock and integration setup won't be possible now, with "Try again" and "Set up later"
- **AND** "Try again" returns to the network list; "Set up later" skips the Dock and Integration steps and shows Finish

#### Scenario: Skip
- **WHEN** Skip is pressed
- **THEN** the Dock step is shown

#### Scenario: Keypad on the WiFi step
- **WHEN** the step is shown
- **THEN** DPAD_UP / DPAD_DOWN move over the networks, then "Join other" (when shown), then Skip; DPAD_MIDDLE selects; on the failure screen DPAD_LEFT / DPAD_RIGHT toggle between Try again (initial) and Set up later; BACK returns to the Profile step

### Requirement: Dock step
The Dock step SHALL show "Dock setup" with the dock discovery start screen, results and a Skip button. Selecting a discovered dock SHALL open the dock setup popup (as in the docks settings) with the network joined in the WiFi step preselected for a Bluetooth dock. A successful setup SHALL close the popup and advance; a failed or cancelled setup SHALL close the popup and return to the discovery start screen. Skip SHALL advance.

#### Scenario: Keypad on the Dock step
- **WHEN** the step is shown
- **THEN** DPAD_UP / DPAD_DOWN walk the start-screen controls or the found docks and end on Skip; DPAD_MIDDLE activates; BACK stops the discovery and returns to the WiFi step; BACK inside the setup popup cancels the setup

### Requirement: Integration step
The Integration step SHALL show "Integration setup" and start the driver discovery on entry, listing the found drivers above a button labelled "Skip", or "Next" once an integration has been set up in this step. Selecting a driver SHALL open the integration setup popup (as in the integrations settings); closing it returns to the list. Skip / Next SHALL advance to Finish.

#### Scenario: Keypad on the Integration step
- **WHEN** the step is shown
- **THEN** DPAD_UP / DPAD_DOWN walk the found drivers and end on Skip / Next; DPAD_MIDDLE activates; BACK stops the discovery and returns to the Dock step; BACK inside the setup popup cancels the setup

### Requirement: Finish step
The Finish step SHALL show "You're all set – You can add integrations or change configuration via the Web configurator." and, for an unrestricted profile, a box with the web configurator switch ("Web configurator enabled" / "disabled"), the configurator address (tapping toggles between hostname and IP), the 4-digit configurator PIN with a button that generates a new PIN, and a QR code of the address, all shown only while the configurator is enabled. The page SHALL scroll so Done is reachable in every language. Done SHALL end the onboarding and show the "Useful tips" help overlay.

#### Scenario: Keypad on the Finish step
- **WHEN** the step is shown
- **THEN** the selection starts on the web configurator switch (or on Done for a restricted profile); DPAD_DOWN reaches the new-PIN button and then Done; the focused control is scrolled into view; BACK does nothing

#### Scenario: Tips after Done
- **WHEN** the help overlay is shown right after the onboarding
- **THEN** DPAD_LEFT / DPAD_RIGHT only move between the tips and do not switch the pages behind them; OK, BACK or HOME closes the tips

### Requirement: Popups on a step own the input
A list or popup opened on a step (language, country and timezone lists, the terms popup, the profile form and list, the WiFi join dialogs, the dock and integration setup popups) SHALL take the key input while it is visible and give it back when it closes; BACK on such a popup SHALL close or cancel the popup and not leave the step behind it. A selection list SHALL be hidden when its step is left and shown again when the step is entered.

#### Scenario: One key press, one step
- **WHEN** a key press moves to the next step
- **THEN** the same key press does not activate a control of the new step
