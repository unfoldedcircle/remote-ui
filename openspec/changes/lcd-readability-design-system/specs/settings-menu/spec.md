## MODIFIED Requirements

### Requirement: Opening the settings
The settings overlay SHALL open from the gear icon of the pull-down menu (pull the current page down past its header plus 100 px) and, as the profile page variant with header rows, by tapping the profile icon in the status bar or by choosing "Profile & settings" in the page menu (a long press on HOME, see `pages`), which is the way in with the keypad. The overlay fades in over 200 ms and takes the input while it is open.

#### Scenario: Closing the overlay
- **WHEN** the back arrow (or the X of the profile page) is tapped, or HOME is pressed on any level
- **THEN** the whole settings overlay closes and the main screen takes the input again

#### Scenario: BACK on the top level
- **WHEN** BACK is pressed on the top-level menu
- **THEN** the overlay closes (after closing an enlarged QR code first, on the profile page)

### Requirement: Wifi & Bluetooth page
The page SHALL offer "Bluetooth" and "WiFi" switches, "Active WiFi scanning" with an interval slider 10–60 seconds in steps of 5, a "WiFi band" selection list, the "Known Networks" list, other networks, "Join other" and "Delete all networks" (asks "Are you sure you want to delete all WiFi networks?" with "Delete all"). Network details are specified by the network capability.

#### Scenario: Whole page by d-pad
- **WHEN** the d-pad walks the page
- **THEN** the switches, the band row, both network lists and the buttons are all reachable in order
- **AND** an empty "Known Networks" list is skipped, and an empty list of other networks starts on "Join other", so a press never lands where nothing is drawn
