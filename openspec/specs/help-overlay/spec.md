# help-overlay Specification

## Purpose

The first-use help overlay: when it is shown, its tips and paging, how it is closed and re-opened, and how it owns the input while visible.

## Requirements

### Requirement: When the tips are shown
The "Useful tips" overlay SHALL be shown right after the onboarding is finished with "Done", and whenever the user picks "Show tips", the last entry of the page menu (long press on HOME on the main screen). It SHALL be shown over the main screen or, when the profile has no pages, over the "no page" screen. It SHALL NOT be shown at any other start-up, and it SHALL NOT be reachable from the settings or with a restricted profile, whose long press on HOME only shows "Profile is restricted".

#### Scenario: End of the onboarding
- **WHEN** the user presses "Done" on the final onboarding step
- **THEN** the main screen (or the "no page" screen) is loaded with the tips overlay on top, showing the first tip

#### Scenario: Re-open from the page menu
- **WHEN** HOME is held for 800 ms on the main screen of an unrestricted profile and "Show tips" is selected
- **THEN** the tips overlay opens on the first tip

#### Scenario: Later start-ups
- **WHEN** the remote restarts after the onboarding was completed
- **THEN** the main screen is shown without the tips overlay

### Requirement: Nothing about the tips is persisted
Whether the tips are open SHALL be kept only in memory. No "tips seen" flag and no last tip position SHALL be stored; every opening SHALL start on the first tip, and a restart SHALL never show the tips on its own.

#### Scenario: Restart while the tips are open
- **WHEN** the UI restarts while the tips overlay is open
- **THEN** the tips are not shown again after the restart

#### Scenario: Re-opening after closing on the last tip
- **WHEN** the tips are closed on the third tip and opened again from the page menu
- **THEN** the first tip is shown

### Requirement: Overlay presentation
The overlay SHALL cover the whole screen, including the status bar, with a black layer at 95 % opacity so the screen behind only shimmers through, and SHALL swallow all touches meant for the screen behind. A navigation row SHALL sit 10 px above the bottom edge with a left arrow (20 px from the left), a "Close" button in the centre and a right arrow (20 px from the right). The left arrow SHALL be invisible and inert on the first tip, the right arrow on the last tip. Each tip SHALL show a title (30 px, primary font) 20 px from the top and left, an optional body text (24 px, secondary font) 20 px below it, and markers pointing at the UI element it describes.

#### Scenario: First tip
- **WHEN** the overlay opens
- **THEN** only the right arrow and "Close" are visible in the navigation row

#### Scenario: Tap on the dimmed area
- **WHEN** the user taps where a page tile shimmers through
- **THEN** the tile does not react

### Requirement: Tips and their order
The overlay SHALL contain exactly three tips in this order: (1) title "Useful tips", text "Check out a few useful tips on where to find important elements of the UI." followed by a blank line and "For more tips visit unfoldedcircle.com/support"; (2) title "Tap here to open settings", right-aligned with a 60 px right margin, with a 40 px round marker in the top right corner where the settings button sits; (3) title "Status bar" placed 60 px from the top, text "Battery level, WiFi connection problem, software update indicator and the page title may appear here." followed by a blank line and "You can also tap the status bar to scroll to the top of a page.", with a 30 px high rounded bar marker across the top of the screen. All titles, texts and the "Close" label SHALL be translatable UI strings shown in the current UI language; the URL SHALL stay untranslated.

#### Scenario: German UI
- **WHEN** the tips are opened with the UI language set to Deutsch
- **THEN** all three tips and the Close button are shown in German

#### Scenario: Settings tip
- **WHEN** the user moves to the second tip
- **THEN** "Tap here to open settings" is shown next to a round marker in the top right corner

### Requirement: Paging between tips
The user SHALL move between tips by swiping horizontally, by tapping the left or right arrow, or with DPAD_LEFT / DPAD_RIGHT. Paging SHALL NOT wrap: DPAD_LEFT on the first tip and DPAD_RIGHT on the last tip SHALL do nothing.

#### Scenario: Keypad paging
- **WHEN** DPAD_RIGHT is pressed twice on the first tip
- **THEN** the "Status bar" tip is shown and a third DPAD_RIGHT does nothing

#### Scenario: Swipe back
- **WHEN** the user swipes right on the second tip
- **THEN** the first tip is shown and the left arrow disappears

### Requirement: Closing the tips
The overlay SHALL close when "Close" is tapped, or on the press of DPAD_MIDDLE, BACK or HOME, from any tip. Closing SHALL remove the overlay, release the input and leave the screen behind unchanged; HOME SHALL NOT additionally act on the main screen.

#### Scenario: OK closes
- **WHEN** DPAD_MIDDLE is pressed on the second tip
- **THEN** the overlay closes and the main screen owns the input again

#### Scenario: HOME closes
- **WHEN** HOME is pressed while the tips are shown
- **THEN** the overlay closes and the current page is not scrolled or changed by that press

### Requirement: Input ownership while the tips are shown
The overlay SHALL take the input when it is created and keep it while it is shown: whenever the main screen or "no page" screen that hosts it becomes the input owner while the tips are still open, the overlay SHALL take the input back on the next event-loop turn. The screen behind SHALL NOT react to DPAD_LEFT / DPAD_RIGHT, DPAD_UP / DPAD_DOWN or DPAD_MIDDLE while the tips are shown.

#### Scenario: Tips after the onboarding
- **WHEN** the tips appear together with the freshly loaded main screen, which takes the input after the overlay
- **THEN** the overlay takes the input back and DPAD_LEFT / DPAD_RIGHT page through the tips instead of switching pages

#### Scenario: Tips opened from the page menu
- **WHEN** "Show tips" is chosen and the page menu closes, handing the input back to the main screen
- **THEN** the overlay owns the input and DPAD_RIGHT shows the next tip
