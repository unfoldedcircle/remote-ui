# docks Specification

## Purpose

How docks are listed, discovered, set up, renamed, re-keyed, dimmed, connected, deleted and factory reset from the remote, and how dock state and update events are shown.

## Requirements

### Requirement: Configured docks are loaded from the core
The UI SHALL load the list of configured docks from the core whenever the connection is established, 100 docks per page, and keep it up to date from the core's `dock_change` (added / changed / deleted) and `dock_state_change` events. The page count SHALL be the total `count` of the latest answer divided by the page size of the request (100), rounded up, and at least 1; the `limit` field of an answer holds the number of docks in that page and SHALL NOT be used as the page size. The next page SHALL be requested only while the page just received is below the page count and contained at least one dock. After the load the list SHALL match the core's answer: a dock that is already known SHALL be updated as for a changed dock, including its state; an unknown dock SHALL be added; a dock that is in no page of the answer SHALL be removed as for a deleted dock. A new load SHALL ignore the answers that still arrive for an earlier load. A dock carries id, name, custom WebSocket URL, active flag, model, revision, serial, connection type, firmware version, state (`IDLE`, `CONNECTING`, `ACTIVE`, `RECONNECTING`, `ERROR`), learning-active flag, description and LED brightness.

#### Scenario: Dock added by the core
- **WHEN** the core reports a new dock (e.g. after a setup in the web configurator)
- **THEN** it appears in the dock list without a reload

#### Scenario: Dock changed by the core
- **WHEN** the core reports a changed dock
- **THEN** the active and learning flags are always updated, and name, custom URL, connection type, version, description and LED brightness are updated only when the event carries a non-empty value (brightness only when not -1)

#### Scenario: Dock state event
- **WHEN** the core reports a new state for a known dock
- **THEN** the dock's state text and image opacity change accordingly

#### Scenario: Dock update events
- **WHEN** the core sends a `dock_update_change` event
- **THEN** the UI ignores it; no update progress is shown anywhere

#### Scenario: Docks fit on one page
- **WHEN** the core reports 2 docks for a request of 100 and the first page carries both
- **THEN** no second page is requested

#### Scenario: Dock deleted while the core was down
- **WHEN** the app reconnects after a core restart and a dock it lists is no longer configured in the core
- **THEN** the dock is removed from the dock list

#### Scenario: Dock changed while the core was down
- **WHEN** the app reconnects after a core restart and a known dock has a new name or state in the core
- **THEN** the dock's entry shows the new name and state, without a second entry for the same dock

#### Scenario: More than 100 docks
- **WHEN** the core reports more docks than fit on one page
- **THEN** the following pages are requested until the last one, and only docks missing from every page are removed

### Requirement: Dock list shows each dock with state and image
The "Docks" settings page SHALL show one 300 px card per dock with the dock image, a check (or X in state `ERROR`) icon, the name, the state text "Active", "Connecting", "Error", "Idle" or "Reconnecting", and in state `ERROR` the red line "Something is wrong". The image is shown at 80 % opacity in states `ACTIVE` and `IDLE` and at 25 % otherwise. A green dot on the image marks a dock whose IR learning is active. A card offers an "Identify" button in states `ACTIVE` / `IDLE` and a "Connect" button in state `ERROR`; both are touch-only.

#### Scenario: Dock image per model
- **WHEN** the dock model is `UCD2`
- **THEN** the Dock Two image is shown
- **AND** for model `UCD3` the image is chosen from the last two serial characters: colour `D` (dark) or `S` (silver) and type `C` (charging) or `N` (non-charging); any other combination, a short serial or an unknown model shows the dark charging Dock 3 image

#### Scenario: Identify from the list
- **WHEN** the user taps Identify
- **THEN** an identify command is sent to the dock and the on-screen dot blinks blue, orange, green and red twice

#### Scenario: Identify or connect fails
- **WHEN** the core rejects the command
- **THEN** the core's error message is shown as a notification

### Requirement: Docks settings page keypad walk
DPAD_UP / DPAD_DOWN SHALL move the selection through the dock list; DPAD_DOWN past the last dock selects the "Add a new dock" sheet and DPAD_UP from there returns to the list. DPAD_MIDDLE opens the selected dock's details or the sheet. When a dock below the selection is removed, the selection stays on the last dock. The Identify / Connect buttons of a card are not reachable by keypad; the same actions are reachable in the dock details.

#### Scenario: Empty dock list
- **WHEN** no dock is configured and DPAD_MIDDLE is pressed on the list
- **THEN** nothing happens

### Requirement: Dock discovery in the "Add a new dock" sheet
The "Add a new dock" sheet SHALL open on a start screen with the text "Tap discover to search for docks on your network or via Bluetooth. If you would like to wirelessly setup a new dock, make sure it's in close proximity to the remote." and a "Discover" button. When Bluetooth is disabled the screen SHALL additionally show the red hint "Bluetooth is disabled. Discovery limited to network only." with a Bluetooth switch that enables it. Discover SHALL clear the previous results and ask the core to start a discovery over Bluetooth and network for new docks only, with the core's default timeout of 30 s. Closing the sheet SHALL stop the discovery.

#### Scenario: Discovery results
- **WHEN** the core reports a discovered dock that is not yet configured
- **THEN** it is listed with an ethernet icon and its friendly name (network discovery) or a Bluetooth icon and its id (Bluetooth discovery), plus its address; a dock reported twice is listed once
- **AND** the header shows "Discovering" with a spinner while the discovery runs and "N dock(s) found" once it stops; tapping the header starts a new discovery

#### Scenario: Discovery cannot be started or stopped
- **WHEN** the core rejects the start or stop request
- **THEN** an actionable warning "Failed to start (stop) dock discovery – There was an error starting (stopping) dock discovery: <message>" with a "Try again" action is shown

#### Scenario: Keypad walk of the discovery
- **WHEN** the start screen is shown
- **THEN** the selection starts on its first control (the Bluetooth switch if shown, otherwise Discover); DPAD_UP / DPAD_DOWN move between them and DPAD_MIDDLE activates the selected control
- **AND** once results are listed the first result is selected automatically; DPAD_UP / DPAD_DOWN move over the results and DPAD_MIDDLE starts the setup of the selected dock; BACK / HOME stop the discovery

### Requirement: Selecting a discovered dock opens the setup popup
Selecting a discovered dock SHALL stop the discovery and open the full-screen "Dock setup" popup with a header card showing an ethernet icon and the friendly name (network dock) or a Bluetooth icon and the dock id (Bluetooth dock) with its address. The form SHALL offer a "Name" field pre-filled with the friendly name or id, an optional "Password" field (masked, characters shown for 1 s), and a WiFi row.

#### Scenario: Bluetooth dock needs WiFi
- **WHEN** the dock was discovered over Bluetooth
- **THEN** the WiFi row is labelled "Required", Next is disabled (dimmed to 30 %) until a network is selected

#### Scenario: Network dock
- **WHEN** the dock was discovered over the network
- **THEN** the WiFi row is labelled "Optional" and Next is enabled right away

#### Scenario: Setup during the onboarding
- **WHEN** the popup opens during the onboarding for a Bluetooth dock
- **THEN** the network the remote joined last (SSID and password) is preselected as the dock's WiFi network

### Requirement: WiFi page of the dock setup
Tapping "Add WiFi network" (or the selected network row) SHALL open the "Select WiFi network" page and start a network scan whose status is polled every 10 s. Selecting an encrypted network asks for its password in a popup; an open network is taken without a password; "Join other" opens the manual network dialog. A selection SHALL return to the form with the SSID shown in the "Selected WiFi network" row, a lock icon when a password was given, and an X that clears the selection. The back arrow or BACK on the WiFi page SHALL return to the form without a change and stop the scan.

#### Scenario: Keypad walk of the WiFi page
- **WHEN** the WiFi page opens by keypad
- **THEN** the selection starts on the network list; DPAD_UP from the first network reaches the back arrow
- **AND** returning with a network puts the selection on Next; returning without one puts it on the page's first field

#### Scenario: Keypad walk of the form
- **WHEN** the form opens by keypad
- **THEN** the selection starts in the Name field; Return in Name moves to Password, Return in Password moves to the WiFi row, DPAD_DOWN from the WiFi row reaches Next (or Cancel while Next is disabled), DPAD_LEFT from Next reaches Cancel, DPAD_RIGHT from the selected-network row reaches its clear X
- **AND** the on-screen keyboard follows the text fields and is hidden on the WiFi rows

### Requirement: Dock setup flow with the core
Next SHALL show the blocking waiting screen and create the setup with `create_dock_setup` (dock id, friendly name, discovery type `BT` or `NET`); on a non-error response the UI SHALL start it with `start_dock_setup` carrying name, password and, when selected, the WiFi SSID and password. A `start_dock_setup` request that is rejected or not answered SHALL fail the setup right away, like a rejected create request. The UI SHALL follow `dock_setup_change` events of the core: a `START` event without error and `CONFIGURING` / `UPLOADING` / `RESTARTING` states are progress only, a `STOP` event with state `OK` finishes successfully, and any event with state `ERROR` fails with the error code (`NOT_FOUND`, `CONNECTION_ERROR`, `CONNECTION_REFUSED`, `AUTHORIZATION_ERROR`, `TIMEOUT`, `ABORT`, `PERSISTENCE_ERROR`, `OTHER`) as text.

#### Scenario: Success
- **WHEN** the `STOP` event with state `OK` arrives
- **THEN** the success animation is played, the finish step shows "You're all set – The dock has been added successfully. – <name> is ready to blast IR codes." with a Done button
- **AND** the "Add a new dock" sheet behind the popup closes

#### Scenario: Failure
- **WHEN** an event or a response reports state `ERROR`, or the create or the start request is rejected
- **THEN** the failure animation is played and the finish step shows "Oops – Something went wrong while setting up the dock. – ERROR: <error code or message>" with a "Try again" button

#### Scenario: Progress states
- **WHEN** an event with state `CONFIGURING`, `UPLOADING` or `RESTARTING` arrives
- **THEN** the state text is translated ("Configuring", "Restarting", "Uploading") but the waiting screen does not display it; no progress is visible to the user

#### Scenario: Start request rejected
- **WHEN** the `start_dock_setup` request itself is rejected by the core (e.g. a validation error) or times out
- **THEN** the waiting screen closes right away and the failure step shows the core's message, instead of the waiting screen staying open until its 180 s timeout

#### Scenario: Try again
- **WHEN** "Try again" or BACK is pressed on the failure page
- **THEN** the setup popup closes; a new attempt requires selecting the dock again from the discovery (a fresh discovery is needed as the results were cleared)

#### Scenario: Done
- **WHEN** Done or BACK is pressed on the success page
- **THEN** the setup popup closes

### Requirement: Cancelling a dock setup
Cancel on the form, BACK on the form or HOME SHALL hide the waiting screen, send `stop_dock_setup` for the dock and close the popup. HOME additionally returns to the home screen. A rejected stop SHALL show the core's message as a notification, except for a 404 (no setup running).

#### Scenario: BACK on the WiFi page
- **WHEN** BACK is pressed while the WiFi page is shown
- **THEN** only the WiFi page closes; the setup is not cancelled

### Requirement: Dock details popup
Tapping a dock card (or DPAD_MIDDLE on the selected card) SHALL open a full-screen details popup that first refreshes the dock from the core, showing the dock image and name ("Tap to edit name"), the "Something is wrong" line in state `ERROR`, Identify (states `ACTIVE` / `IDLE`) or Connect (state `ERROR`), the fields State, Connection type, Service name, Custom IP or URL ("Not set" when empty), Firmware version ("N/A" when unknown), a "Led brightness" slider from 0 to 100 in steps of 1, the rows "Change password", "Change WiFi settings" and "Factory reset", and a "Delete dock" drawer at the bottom. Name editing, the slider and the three rows are enabled only in states `ACTIVE` and `IDLE` and shown at 30 % opacity otherwise.

#### Scenario: Refresh fails
- **WHEN** the core cannot deliver the dock's current data
- **THEN** the notification "There was an error while getting the latest dock data" is shown and the cached data is used

#### Scenario: Keypad walk of the details
- **WHEN** the popup opens by keypad
- **THEN** the selection starts on the name row (or on Connect / the delete opener when the name cannot be edited)
- **AND** DPAD_DOWN walks name → Identify / Connect → LED slider → Change password → Change WiFi settings → Factory reset → Delete, skipping invisible or disabled rows; the focused row is scrolled into view; BACK / HOME close the popup

#### Scenario: LED brightness
- **WHEN** the user drags the slider and releases it, or presses DPAD_LEFT / DPAD_RIGHT on it
- **THEN** the new brightness is sent to the dock (each keypad step immediately); a rejected command shows the core's message

#### Scenario: Change WiFi settings
- **WHEN** the user activates "Change WiFi settings"
- **THEN** the notification "Not implemented yet" is shown

### Requirement: Rename a dock
The name row SHALL open a "Rename dock" dialog pre-filled with the current name, with Cancel and Rename. Rename with an empty field marks the field as erroneous; otherwise the waiting screen is shown and the name is sent to the core. On success the dialog closes; on failure the field shows "There was an error. Try again".

#### Scenario: Keypad in the rename dialog
- **WHEN** the dialog opens
- **THEN** the field has the focus and the keyboard is shown; DPAD_MIDDLE on the field submits once, DPAD_DOWN reaches Cancel, DPAD_RIGHT from Cancel reaches Rename, BACK / HOME close the dialog

### Requirement: Change the dock password
"Change password" SHALL open a "Change password" dialog with a masked field (characters shown for 1 s), Cancel and Change. Change with an empty field marks the field as erroneous; otherwise the waiting screen is shown and the password is sent to the core. On success the dialog closes and the field is cleared; on failure the field shows "There was an error. Try again". The keypad walk equals the rename dialog.

#### Scenario: Cancel
- **WHEN** Cancel or BACK is pressed
- **THEN** the field is cleared, the keyboard hidden and the dialog closed without a request

### Requirement: Factory reset a dock
"Factory reset" SHALL show an actionable warning "Factory reset – Are you sure you want to factory reset <name>?" with a "Reset" action. Confirming sends the reset command, closes the details popup and resets it; a rejected command shows the core's message.

#### Scenario: Warning dismissed
- **WHEN** the warning is dismissed without "Reset"
- **THEN** nothing is sent and the details stay open

### Requirement: Delete a dock with a deliberate second step
The delete drawer SHALL ask "Are you sure you want to delete <name>?" with Cancel and Delete. Opened by keypad the selection starts on Cancel; DPAD_LEFT / DPAD_RIGHT move between Cancel and Delete, DPAD_MIDDLE activates, BACK / HOME close the drawer. Delete sends the delete request and closes the details popup; a rejected request shows the core's message. While the drawer is open the page is dimmed and not scrollable.

#### Scenario: Dock deleted by the core
- **WHEN** the core reports the dock as deleted
- **THEN** it is removed from the dock list

### Requirement: Connect a dock in error state
"Connect" in the list card or the details SHALL send a connect request for the dock; a rejected request shows the core's message. The dock state is updated only through the subsequent `dock_state_change` event.

#### Scenario: Connect succeeds
- **WHEN** the core accepts the request
- **THEN** no confirmation is shown; the card changes once the state event reports `CONNECTING` / `ACTIVE`
