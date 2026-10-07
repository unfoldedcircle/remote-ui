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
