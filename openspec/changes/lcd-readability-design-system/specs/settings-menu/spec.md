## MODIFIED Requirements

### Requirement: Settings submenu
"Settings" SHALL list, in order: "Display & Brightness", "User interface", "Touch Slider", "Sound & Haptic", "Voice Control", "Power Saving", "Wifi & Bluetooth", "Localization", "Administrator PIN", "Factory reset". Opening it re-requests the configuration from the core. The list scrolls when it overflows, keeping the selection between 15% and 85% of its height.

#### Scenario: Entry chosen
- **WHEN** an entry is chosen
- **THEN** its page loads on the third level

### Requirement: User interface page
The page SHALL offer switches: "Inverted button behavior" (short press opens the control screen, long press toggles); "Show battery percentage"; "Show battery indicator everywhere"; "Activities on pages" (activity bar in the page header); "Open activities started with the API"; "Zoom media image" (crop artwork instead of fit); "Coverflow in media browser".

#### Scenario: Activity bar toggled
- **WHEN** "Activities on pages" is switched
- **THEN** the page headers on the main screen resize immediately

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

## ADDED Requirements

### Requirement: Addresses on the About page are shown in full
The Wi-Fi address and the Bluetooth address on the About page SHALL be shown in full: on the line of their key
when both fit, otherwise on their own line under the key. They SHALL NOT be cut off, in any language.

#### Scenario: Addresses on a Remote 3
- **WHEN** the About page is shown on the 480 px wide screen of a Remote 3
- **THEN** both addresses are shown in full, each on its own line under its key

### Requirement: The About page shows its first entry
On both remotes the information of the About page and its first entry SHALL fit on the screen together, so the
page shows without scrolling that entries follow the information, and returning to the first entry shows the
information from the top.

#### Scenario: About page on a Remote 3
- **WHEN** the About page opens on the 800 px high screen of a Remote 3
- **THEN** "Regulatory" is shown in full below the information, without scrolling

### Requirement: The About page scrolls back to its information
When the keypad selection reaches the first entry of the About page, or DPAD_UP is pressed on it, the page SHALL
scroll up as far as the selected entry stays visible, so the information above the entries is shown again.

#### Scenario: Back up with the keypad
- **WHEN** the selection is moved down to "Licenses" and back up to "Regulatory"
- **THEN** the page scrolls up as far as "Regulatory" stays visible, which is the top of the page with the model
  number

#### Scenario: After scrolling by touch
- **WHEN** the page was scrolled down by touch and DPAD_UP is pressed while "Regulatory" is selected
- **THEN** the page scrolls up the same way
