## MODIFIED Requirements

### Requirement: Tips and their order
The overlay SHALL contain exactly three tips in this order: (1) title "Useful tips", text "Check out a few useful tips on where to find important elements of the UI." followed by a blank line and "For more tips visit unfoldedcircle.com/support"; (2) title "Profile & settings", right-aligned with a 60 px right margin, text "Open them with this button, or hold HOME and choose Profile & settings.", with a 40 px round marker in the top right corner where the profile button sits; (3) title "Status bar" placed 60 px from the top, text "Battery level, WiFi connection problem, software update indicator and the page title may appear here." followed by a blank line and "Press HOME, or touch the status bar, to scroll to the top of a page.", with a 30 px high rounded bar marker across the top of the screen. All titles, texts and the "Close" label SHALL be translatable UI strings shown in the current UI language; the URL SHALL stay untranslated.

#### Scenario: German UI
- **WHEN** the tips are opened with the UI language set to Deutsch
- **THEN** all three tips and the Close button are shown in German

#### Scenario: Settings tip
- **WHEN** the user moves to the second tip
- **THEN** "Profile & settings" is shown next to a round marker in the top right corner

### Requirement: Closing the tips
The overlay SHALL close when "Close" is tapped, or on the press of DPAD_MIDDLE, BACK or HOME, from any tip. As DPAD_MIDDLE always closes, "Close" SHALL be drawn with the selection ring whenever the keypad is active. Closing SHALL remove the overlay, release the input and leave the screen behind unchanged; HOME SHALL NOT additionally act on the main screen.

#### Scenario: OK closes
- **WHEN** DPAD_MIDDLE is pressed on the second tip
- **THEN** the overlay closes and the main screen owns the input again

#### Scenario: HOME closes
- **WHEN** HOME is pressed while the tips are shown
- **THEN** the overlay closes and the current page is not scrolled or changed by that press

### Requirement: Overlay presentation
The overlay SHALL cover the whole screen, including the status bar, with a black layer at 95 % opacity so the screen behind only shimmers through, and SHALL swallow all touches meant for the screen behind. A navigation row SHALL sit 10 px above the bottom edge with a left arrow (20 px from the left), a "Close" button in the centre and a right arrow (20 px from the right). The left arrow SHALL be invisible and inert on the first tip, the right arrow on the last tip. Each tip SHALL show a title (30 px, primary font) 20 px from the top and left, an optional body text in the help role 20 px below it, and markers pointing at the UI element it describes.

#### Scenario: First tip
- **WHEN** the overlay opens
- **THEN** only the right arrow and "Close" are visible in the navigation row

#### Scenario: Tap on the dimmed area
- **WHEN** the user taps where a page tile shimmers through
- **THEN** the tile does not react
