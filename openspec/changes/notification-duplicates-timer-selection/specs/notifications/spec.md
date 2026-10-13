## MODIFIED Requirements

### Requirement: Toasts replace each other without a queue
Only one toast SHALL be visible at a time and toasts SHALL NOT be queued. A toast created while another is visible SHALL replace the visible message text and colour in place, without a new entry animation, and SHALL restart the 4000 ms timer, so the newer message is shown for the full 4000 ms. A toast created while the toast is still fading in SHALL get its 4000 ms from the moment it is fully shown.

#### Scenario: Two errors in quick succession
- **WHEN** a warning toast opens and a neutral toast is created 3 s later
- **THEN** the neutral message replaces the warning text and colour and the toast closes 4 s later
- **AND** the first message is not shown again

### Requirement: Actionable notifications stack and de-duplicate
Actionable notifications SHALL be kept on a stack: a new one SHALL be shown on top of those already open, and dismissing the top one SHALL reveal the previous one; the overlay closes when the last one is dismissed. A new actionable notification whose title and message both equal those of a notification still on the stack SHALL be dropped silently. A dismissed notification SHALL be discarded and SHALL NOT suppress later notifications with the same title and message.

#### Scenario: Second notification while one is open
- **WHEN** "Low battery" is open and "Connection error" is created
- **THEN** "Connection error" is shown; dismissing it shows "Low battery" again

#### Scenario: Duplicate title
- **WHEN** "Kitchen TV is not responding" is open and another command to the same device times out
- **THEN** no second notification is added

#### Scenario: Same title, different message
- **WHEN** "Error sending the command" / "Kitchen TV is not responding. Error code: 400" is open and a command to
  "Living room light" fails with 400
- **THEN** "Error sending the command" / "Living room light is not responding. Error code: 400" is shown on top;
  dismissing it shows the one for "Kitchen TV"

#### Scenario: Same title after dismissal
- **WHEN** the "Try again" notification of a device was dismissed and a later command to that device times out
- **THEN** a new "is not responding" notification is shown

### Requirement: Actionable notification touch interaction
Tapping the action label SHALL run the action once and dismiss the top notification. Tapping "Cancel" SHALL dismiss it without running the action. Tapping anywhere else on the notification SHALL dismiss it without running the action. The touch area of the action and of "Cancel" SHALL be exactly their button, so a tap on the message dismisses the notification and never runs or cancels the action.

#### Scenario: Try again by touch
- **WHEN** the user taps "Try again" on "<device> is not responding"
- **THEN** the command is sent once more and the notification is dismissed

#### Scenario: Tap on the icon
- **WHEN** the user taps the icon of a confirmation notification
- **THEN** it is dismissed and the confirmed action is not run

#### Scenario: Tap on the message just above the buttons
- **WHEN** the user taps the last line of the message of a confirmation notification, right above its buttons
- **THEN** it is dismissed and the confirmed action is not run

### Requirement: Actionable notification keys
While at least one actionable notification is open it SHALL own the input (taken when the overlay opens, released when it closes). When the overlay opens and whenever a notification is pushed onto an open one, the action SHALL be preselected; the selection outline SHALL be drawn only while the keypad is active. DPAD_LEFT SHALL select "Cancel" when the top notification has an action label; DPAD_RIGHT SHALL select the action. DPAD_MIDDLE SHALL run the action of the top notification and dismiss it when the action is selected (a notification without an action is simply dismissed), and SHALL only dismiss it when "Cancel" is selected. BACK SHALL dismiss the top notification without running its action. HOME SHALL dismiss all open actionable notifications at once.

#### Scenario: Confirm with the keypad
- **WHEN** the "Remove WiFi network" confirmation is open and DPAD_MIDDLE is pressed
- **THEN** the network is removed and the notification is dismissed

#### Scenario: Cancel with the keypad
- **WHEN** DPAD_LEFT and then DPAD_MIDDLE are pressed on a "Factory reset" confirmation
- **THEN** the notification is dismissed and the dock is not reset

#### Scenario: A notification arrives while Cancel is selected
- **WHEN** "Cancel" is selected on an open notification and a second notification with an action arrives
- **THEN** the second one is shown with its action selected, and DPAD_MIDDLE runs that action

#### Scenario: HOME with several notifications
- **WHEN** three actionable notifications are stacked and HOME is pressed
- **THEN** all three are dismissed and the input returns to the screen below
