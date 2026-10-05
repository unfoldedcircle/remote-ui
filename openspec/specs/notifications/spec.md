# notifications Specification

## Purpose

How the UI informs the user: plain and warning toasts, actionable notifications, the notification drawer, and which events produce which kind of notification.

## Requirements

### Requirement: Two kinds of notification
The UI SHALL offer exactly two kinds of notification: a toast, which carries only a message text and is either neutral or a warning, and an actionable notification, which carries a title, a message, an optional icon, an optional action with its label, and is either neutral or a warning. Both kinds SHALL be available on every screen, including the onboarding, and SHALL be drawn above the current screen.

#### Scenario: Toast
- **WHEN** an entity that already is on the page is added to it again
- **THEN** a neutral toast "<entity id> already exists on the page." is shown

#### Scenario: Actionable notification
- **WHEN** "Reorder" is chosen on a page without tiles
- **THEN** the actionable notification "Page is empty" / "There is nothing to reorder. Try adding entities or groups first." is shown

### Requirement: Toast presentation and duration
A toast SHALL appear at the top of the screen, 10 px from the top edge, as wide as the display minus 20 px, with rounded corners, the message in 20 px text with 20 px side margins, and a height that fits the wrapped text plus 30 px. A warning toast SHALL have a red background; a neutral toast the neutral button colour. It SHALL appear with a 200 ms fade and slide down from 100 px above the edge while flipping in from 90 degrees and growing from 90 % scale over 300 ms, SHALL close by itself 4000 ms after it opened, and SHALL fade out in 200 ms. A toast SHALL play no sound or haptic.

#### Scenario: Warning toast lifetime
- **WHEN** a warning toast "Wrong network key" is shown and nothing else happens
- **THEN** it is shown on a red background and fades out 4 s after it appeared

### Requirement: Toasts replace each other without a queue
Only one toast SHALL be visible at a time and toasts SHALL NOT be queued. A toast created while another is visible SHALL replace the visible message text and colour in place, without a new entry animation and without restarting the 4000 ms timer, so the newer message disappears 4000 ms after the first toast opened.

#### Scenario: Two errors in quick succession
- **WHEN** a warning toast opens and a neutral toast is created 3 s later
- **THEN** the neutral message replaces the warning text and colour and the toast closes about 1 s later
- **AND** the first message is not shown again

### Requirement: Toast dismissal and input
A toast SHALL close when the user presses anywhere outside it, and that press SHALL still reach the screen below. A press on the toast itself SHALL do nothing. A toast SHALL NOT take the input ownership and SHALL NOT react to any physical button.

#### Scenario: Tap outside
- **WHEN** a toast is visible and the user taps a page tile
- **THEN** the toast closes and the tile reacts to the tap

#### Scenario: Keys while a toast is visible
- **WHEN** DPAD_DOWN is pressed while a toast is visible
- **THEN** the screen below handles the key and the toast stays until its timer ends

### Requirement: Actionable notification presentation
An actionable notification SHALL cover the whole screen with a gradient from transparent at the top to black from 60 % of the height downwards, and SHALL show, from the bottom up: the action label (bold 26 px, right half) and "Cancel" (bold 26 px, left half) 30 px above the bottom edge, both only when an action label is given; the message (24 px, 70 % opacity) 40 px above them; the title (40 px) 20 px above the message; and a 140 px icon above the title, which SHALL be a warning triangle when no icon is given. A 4 px bar along the bottom edge SHALL pulse (1000 ms fade out, 1000 ms fade in, 2000 ms pause, repeated). Warning notifications SHALL draw the icon and the bar in red; neutral ones draw the icon in off-white and the bar in the highlight colour. The notification SHALL fade in over 300 ms and out over 200 ms, and SHALL stay until the user dismisses it; it never closes on a timer.

#### Scenario: Warning with an action
- **WHEN** "Delete all networks" is requested in the WiFi settings
- **THEN** a notification with a red warning icon, "Delete all networks", "Are you sure you want to delete all WiFi networks?", "Cancel" on the left and "Delete all" on the right is shown with a pulsing red bar

#### Scenario: Neutral notification without an action
- **WHEN** "Select entities" / "Please select entities to add by tapping in the list." is shown
- **THEN** no action label and no "Cancel" are shown and the notification stays until dismissed

### Requirement: Actionable notifications stack and de-duplicate
Actionable notifications SHALL be kept on a stack: a new one SHALL be shown on top of those already open, and dismissing the top one SHALL reveal the previous one; the overlay closes when the last one is dismissed. A new actionable notification whose title equals the title of any notification still on the stack SHALL be dropped silently. A dismissed notification SHALL be discarded and SHALL NOT suppress later notifications with the same title.

#### Scenario: Second notification while one is open
- **WHEN** "Low battery" is open and "Connection error" is created
- **THEN** "Connection error" is shown; dismissing it shows "Low battery" again

#### Scenario: Duplicate title
- **WHEN** "Kitchen TV is not responding" is open and another command to the same device times out
- **THEN** no second notification is added

#### Scenario: Same title after dismissal
- **WHEN** the "Try again" notification of a device was dismissed and a later command to that device times out
- **THEN** a new "is not responding" notification is shown

### Requirement: Actionable notification touch interaction
Tapping the action label SHALL run the action once and dismiss the top notification. Tapping "Cancel" SHALL dismiss it without running the action. Tapping anywhere else on the notification SHALL dismiss it without running the action. The touch areas of the action label and of "Cancel" SHALL be squares as tall as they are wide (label width + 40 px) centred on the label, so they extend well above the labels over the lower part of the message.

#### Scenario: Try again by touch
- **WHEN** the user taps "Try again" on "<device> is not responding"
- **THEN** the command is sent once more and the notification is dismissed

#### Scenario: Tap on the icon
- **WHEN** the user taps the icon of a confirmation notification
- **THEN** it is dismissed and the confirmed action is not run

### Requirement: Actionable notification keys
While at least one actionable notification is open it SHALL own the input (taken when the overlay opens, released when it closes). On opening, the action SHALL be preselected; the selection outline SHALL be drawn only while the keypad is active. DPAD_LEFT SHALL select "Cancel" when the top notification has an action label; DPAD_RIGHT SHALL select the action. DPAD_MIDDLE SHALL run the action of the top notification and dismiss it when the action is selected (a notification without an action is simply dismissed), and SHALL only dismiss it when "Cancel" is selected. BACK SHALL dismiss the top notification without running its action. HOME SHALL dismiss all open actionable notifications at once.

#### Scenario: Confirm with the keypad
- **WHEN** the "Remove WiFi network" confirmation is open and DPAD_MIDDLE is pressed
- **THEN** the network is removed and the notification is dismissed

#### Scenario: Cancel with the keypad
- **WHEN** DPAD_LEFT and then DPAD_MIDDLE are pressed on a "Factory reset" confirmation
- **THEN** the notification is dismissed and the dock is not reset

#### Scenario: HOME with several notifications
- **WHEN** three actionable notifications are stacked and HOME is pressed
- **THEN** all three are dismissed and the input returns to the screen below

### Requirement: No notification history
The UI SHALL NOT keep a history of notifications: there SHALL be no notification drawer or list, no unread count and no badge, and a toast or actionable notification that was closed SHALL NOT be retrievable. The red dot in the status bar SHALL indicate the core connection only (see `core-connection`), not pending notifications.

#### Scenario: After dismissing
- **WHEN** an actionable notification is dismissed
- **THEN** it cannot be shown again from anywhere in the UI

### Requirement: Which events produce which kind
The UI SHALL use:
- warning toasts for rejected requests and local refusals: settings and localisation changes rejected by the core, dock operations (identify, connect, delete, factory reset, rename, password, LED brightness, stop setup), WiFi commands and "Wrong network key", profile switch, add and page operations, entity rename / icon / configuration / deletion / state fix, integration driver and discovery operations and setup start errors, power off and reboot failures, configuration load failures ("Error while loading configuration. Trying again."), "Profile is restricted", "<name> is unavailable" and "The pin doesn't match. Try again.";
- neutral toasts for duplicates on a page or group ("<entity id> already exists on the page." / "… in this group.") and "Not implemented yet";
- neutral actionable notifications for input hints ("Select entities", "Page is empty", "Select a security option", "Touch slider is not available.") and for an entity command that timed out or could not be delivered (408 / 503: "<name> is not responding" with "Try again", see `remote-entity`);
- warning actionable notifications for confirmations of destructive actions ("Remove WiFi network" / "Remove", "Delete all networks" / "Delete all", "Factory reset" / "Reset", "Remove entity" / "Remove"), other entity command errors ("Error sending the command"), core connection loss ("Connection error", see `core-connection`), battery warnings (see `power-and-battery`), software update failures ("Update check failed", "Update error", and "Low battery" when an installation is started at 50 % charge or less, see `software-update`), integration connection errors ("<driver> error"), dock discovery start / stop failures with "Try again", an already running integration setup ("Failed to start setup" with "Stop"), media browsing and search errors (with "Retry" for 408 / 503, see `media-player`), an existing profile name during the onboarding ("Profile already exists" with "Choose existing"), profile update errors and the refusal to delete the current profile.
Voice assistant errors SHALL NOT produce notifications (see `voice-assistant`).

#### Scenario: Rejected setting
- **WHEN** the core rejects a new display sleep timeout
- **THEN** a warning toast is shown and no actionable notification

#### Scenario: Destructive confirmation
- **WHEN** "Factory reset" is tapped in a dock's details
- **THEN** a warning actionable notification asks "Are you sure you want to factory reset <dock name>?" with "Reset" and "Cancel"

### Requirement: Source of notification texts
Notifications created for user guidance, confirmations, integration errors, software update errors and entity command errors SHALL use fixed, translatable texts; entity command errors SHALL show only the response code, not a message from the core. Several error notifications SHALL show the message text returned by the core verbatim: as the whole toast (profile switch, dock operations, WiFi commands), after a fixed English prefix that is not translated (e.g. "Error setting language: <message>", "Error adding page: <message>", "Error adding network: <message>", "Error on reboot: <message>"), inside a translated sentence ("There was an error starting dock discovery: <message>"), or as the message of an actionable notification ("Profile update error", media browsing and search errors, falling back to a translated default text when the core sends none). "The pin doesn't match. Try again.", "There was an error while getting the latest dock data" and "Not implemented yet" SHALL be shown in English regardless of the UI language.

#### Scenario: Language rejected by the core
- **WHEN** the core rejects a language change with the message "Invalid language code format"
- **THEN** the warning toast reads "Error setting language: Invalid language code format" in every UI language

#### Scenario: Command error code
- **WHEN** an entity command fails with code 500
- **THEN** the notification reads "Error sending the command" / "<name> is not responding. Error code: 500" without the core's message
