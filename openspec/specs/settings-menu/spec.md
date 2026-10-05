# settings-menu Specification

## Purpose

The settings menu of the remote: how it is opened and left, its structure and scrolling, the options of each settings page, and the about page.

## Requirements

### Requirement: Opening the settings
The settings overlay SHALL open from the gear icon of the pull-down menu (pull the current page down past its header plus 100 px) and, as the profile page variant with header rows, by tapping the profile icon in the status bar. The overlay fades in over 200 ms and takes the input while it is open.

#### Scenario: Closing the overlay
- **WHEN** the back arrow (or the X of the profile page) is tapped, or HOME is pressed on any level
- **THEN** the whole settings overlay closes and the main screen takes the input again

#### Scenario: BACK on the top level
- **WHEN** BACK is pressed on the top-level menu
- **THEN** the overlay closes (after closing an enlarged QR code first, on the profile page)

### Requirement: Top-level menu
The top level SHALL list "Software update", "Settings", "Integrations", "Docks" and "About" (Integrations and Docks are separate capabilities). "Software update" carries a red badge "1" when an update is available; Integrations and Docks carry a badge with their count when greater than zero. For a restricted profile only "About" is listed and the menu does not scroll.

#### Scenario: Entry chosen
- **WHEN** an entry is tapped or DPAD_MIDDLE is pressed on the selected entry
- **THEN** its page loads on the second level with the entry name as title

#### Scenario: Menu overflows
- **WHEN** the menu has more entries than fit
- **THEN** it scrolls, with fade gradients at the top and bottom edges while more entries are hidden, and the d-pad selection is kept in view

### Requirement: Level navigation
The settings SHALL have three levels (menu, page, sub page) shown one at a time without swipe gestures. Each page has a top navigation bar of 60 px with a back arrow and the page title (elided). BACK or the back arrow SHALL go back exactly one level and return the selection to the entry the page was opened from; HOME SHALL close the settings entirely.

#### Scenario: Page loaded
- **WHEN** a page finishes loading
- **THEN** it takes the input; a page with focus-navigated controls starts with its declared initial control focused

### Requirement: Base page behaviour
Every settings page SHALL scroll its content when it is taller than the screen. The control selected with the d-pad is kept on screen, whole row including title and description. At the end of the focus chain DPAD_DOWN scrolls the page on by half its height so trailing text is reachable, and DPAD_UP at the top scrolls back. Switches, checkboxes and buttons are operated with DPAD_MIDDLE.

#### Scenario: Popup over a page
- **WHEN** a selection list or dialog opens over a settings page
- **THEN** the page underneath stops reacting to the d-pad until the popup closes

### Requirement: Settings submenu
"Settings" SHALL list, in order: "Display & Brightness", "User interface", "Touch Slider", "Sound & Haptic", "Voice Control", "Power Saving", "Wifi & Bluetooth", "Localisation", "Administrator PIN", "Factory reset". Opening it re-requests the configuration from the core. The list scrolls when it overflows, keeping the selection between 15% and 85% of its height.

#### Scenario: Entry chosen
- **WHEN** an entry is chosen
- **THEN** its page loads on the third level

### Requirement: Immediate application of settings
Every switch and slider on the settings pages SHALL be sent to the core as soon as it is changed (sliders on release); there is no Save or confirmation. A rejected change SHALL show a warning notification with the core's message and keep the previous value.

#### Scenario: Value changed by the core
- **WHEN** the core reports a configuration change
- **THEN** the page shows the new value

### Requirement: Display & Brightness page
The page SHALL offer: "Auto brightness" switch ("Automatically adjust the display brightness based on ambient lighting conditions."); "Display brightness" slider 5–100; "Button backlight" switch ("When on, button backlight will automatically turn on in a dark room."); "Button backlight brightness" slider 0–100.

#### Scenario: Brightness slider moved
- **WHEN** the display brightness slider is moved
- **THEN** the brightness follows live and the final value is sent on release; on Remote Two the UI additionally dims itself according to the value

### Requirement: User interface page
The page SHALL offer switches: "Inverted button behaviour" (short press opens the control screen, long press toggles); "Show battery percentage"; "Show battery indicator everywhere"; "Activities on pages" (activity bar in the page header); "Open activities started with the API"; "Zoom media image" (crop artwork instead of fit); "Coverflow in media browser".

#### Scenario: Activity bar toggled
- **WHEN** "Activities on pages" is switched
- **THEN** the page headers on the main screen resize immediately

### Requirement: Touch Slider page
The page SHALL offer a "Touch slider" switch ("When off, the touch slider is disabled everywhere and swiping it does nothing.") and four gain sliders, "Volume", "Brightness", "Cover position" and "Seek", each 0.1–2.0 in steps of 0.1 (1.0 = one full swipe covers the whole range), plus a live test area "Test – <name>" driven by the hardware slider for the last touched setting.

#### Scenario: Slider disabled
- **WHEN** the touch slider switch is off
- **THEN** the test area is disabled

### Requirement: Sound & Haptic page
The page SHALL offer "Sound effects" switch, "Sound effects volume" slider 0–100 and "Haptic feedback" switch.

#### Scenario: Volume changed
- **WHEN** the volume slider is released
- **THEN** UI sound effects play at the new volume

### Requirement: Voice Control page
The page SHALL offer a "Microphone" switch ("Disabling the microphone will completely turn it off. You won't be able to use voice assistants."); with the microphone on, a read-only "Voice Assistant" row with the assistant name (or "None selected") and "Profile: <name>" / "No profile selected", the note "Use the Web Configurator to edit voice assistants."; and, when an assistant is selected, a "Speech response" switch.

#### Scenario: Microphone off
- **WHEN** the microphone switch is off
- **THEN** the assistant and speech response rows are hidden

### Requirement: Power Saving page
The page SHALL offer: "Keep WiFi connected in standby" switch; "Retry commands after wakeup" slider 0–10 seconds (0 shown as "Disabled"); "Wakeup sensitivity" slider 0–3 (0 = "Off"); "Display off timeout" slider 10–60 seconds; "Sleep timeout" slider 10–300 seconds (shown as minutes:seconds).

#### Scenario: Values shown
- **WHEN** the page opens
- **THEN** every slider shows the current value from the core with its unit label

### Requirement: Wifi & Bluetooth page
The page SHALL offer "Bluetooth" and "WiFi" switches, "Active WiFi scanning" with an interval slider 10–60 seconds in steps of 5, a "WiFi band" selection list, the "Known Networks" list, other networks, "Join other" and "Delete all networks" (asks "Are you sure you want to delete all WiFi networks?" with "Delete all"). Network details are specified by the network capability.

#### Scenario: Whole page by d-pad
- **WHEN** the d-pad walks the page
- **THEN** the switches, the band row, both network lists and the buttons are all reachable in order

### Requirement: Localisation page
The page SHALL offer rows opening selection lists: "Language" (installed translations shown in their native name), "Country" (sections "Suggested" and "All countries"), "Timezone" (zones of the country plus an "All timezones…" entry), a "24-hour time" switch and "Unit System". Each list preselects the current value; a selection is sent to the core and applied immediately; closing a list with BACK changes nothing.

#### Scenario: Empty or unknown value
- **WHEN** a list would yield an empty or unknown value
- **THEN** it is refused and not sent

### Requirement: Administrator PIN page
The page SHALL show a PIN keypad twice in sequence: enter a 4-digit PIN, then repeat it. Matching PINs are sent with `set_profile_cfg` and the page returns to the submenu on success. Differing PINs show "The pin doesn't match. Try again." and restart on the first keypad; a core error also restarts. DPAD_UP / DOWN / LEFT / RIGHT move the key outline (shown on the first press), DPAD_MIDDLE enters the outlined digit, touch entry shows no outline.

#### Scenario: Outline carried over
- **WHEN** the first PIN was entered with the d-pad
- **THEN** the confirmation keypad starts with the outline on the same key

### Requirement: Factory reset page
The page SHALL show "Resetting will delete all settings, configuration and any information saved on the remote. Data cannot be recovered. Continue?" and a red "Erase everything" button that is reachable by touch only. Pressing it requests a reset token and opens a red full-screen confirmation "Point of no return" / "Confirming factory reset will erase all configuration and data. Data cannot be recovered." with "Confirm" and "Cancel".

#### Scenario: Confirm
- **WHEN** Confirm is tapped
- **THEN** `factory_reset` is sent with the token; a failure shows "Error factory reset: <message>"

#### Scenario: Cancel
- **WHEN** Cancel is tapped, BACK or HOME is pressed, or the area outside is pressed
- **THEN** the confirmation closes and the token is discarded

### Requirement: About page
The About page SHALL list "Model number", "Serial number", "Revision", "Wi-Fi address", "Bluetooth address", "UI version", "Core version" and "System version", followed by the entries "Regulatory", "Terms & conditions", "Warranty information" and "Licenses". The page scrolls as a whole so all entries are reachable on both screen sizes.

#### Scenario: Document page
- **WHEN** Regulatory, Terms & conditions or Warranty information is opened
- **THEN** the document is shown as rich text from the legal resources; DPAD_DOWN / DPAD_UP scroll by 100 px per press, clamped to the content; a relative link inside the document is followed once, http links are ignored

#### Scenario: Licenses
- **WHEN** Licenses is opened
- **THEN** the license text is shown as Markdown split into sections at second-level headings, scrolled the same way

### Requirement: Placeholder pages
The "Activities & macros", "Remotes" and "Colors" pages exist but SHALL NOT be listed in any menu.

#### Scenario: Not reachable
- **WHEN** the menus are browsed
- **THEN** none of these pages can be opened
