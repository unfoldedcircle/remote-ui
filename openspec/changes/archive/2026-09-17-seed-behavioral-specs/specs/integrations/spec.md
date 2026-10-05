## ADDED Requirements

### Requirement: Integration and driver lists are loaded from the core
The UI SHALL load all configured integrations and all integration drivers from the core whenever the core connection is established, paging through the results 100 items per page until every page has been received. For every driver in the list the UI SHALL fetch the full driver details (name, version, icon, description, developer, home page, release date, setup schema, instance count, external flag). Once every driver is known the UI SHALL load the integration status list, which supplies the device state of every integration and the driver state of its driver.

#### Scenario: Lists are refreshed on connect
- **WHEN** the connection to the core is (re-)established
- **THEN** the driver and integration models are cleared and reloaded from page 1
- **AND** the next page is requested as long as the number of loaded pages is lower than the total page count

#### Scenario: Opening the integrations settings page
- **WHEN** the user opens the "Integrations" settings page
- **THEN** the driver list and the integration list are reloaded from the core

#### Scenario: A driver cannot be loaded
- **WHEN** the core rejects the request for a driver's details
- **THEN** the notification "Error getting integration driver" is shown

### Requirement: Integration list shows configured integrations with their state
The integrations settings page SHALL list every configured integration with its icon (a generic puzzle icon when the integration has none), its name in the UI language, its current device state and the version of its driver. External drivers SHALL be marked with a globe icon. Each entry SHALL offer a switch that reflects the `connected` state and connects or disconnects the integration by touch; the switch is enabled only in the states `connected`, `disconnected` and `error`.

#### Scenario: Toggling the switch of a connected integration
- **WHEN** the integration state is `connected` and the user taps the switch
- **THEN** a disconnect command for that integration is sent to the core

#### Scenario: Toggling the switch of a disconnected or failed integration
- **WHEN** the integration state is `disconnected` or `error` and the user taps the switch
- **THEN** a connect command for that integration is sent to the core

#### Scenario: Integration in a transient state
- **WHEN** the integration state is anything other than `connected`, `disconnected` or `error` (e.g. `connecting`)
- **THEN** the switch is shown dimmed and does not react

#### Scenario: Connect or disconnect fails
- **WHEN** the core rejects a connect or disconnect command
- **THEN** the notification "Error while connecting to the integration" or "Error while disconnecting to the integration" is shown

### Requirement: Integration and driver state follow core events
The UI SHALL apply the `integration_device_state` / `integration_driver_state` events of the core to the integration and driver models without a reload, and apply add / change / delete events of integrations and drivers to the lists.

#### Scenario: Device state event
- **WHEN** the core reports a new device state for a known integration
- **THEN** the state shown for that integration changes to the reported value (lower-cased)

#### Scenario: Driver enters an error state
- **WHEN** the core reports a driver state containing "error" that differs from the current state
- **THEN** the driver state is updated
- **AND** outside the onboarding a warning notification "<driver name> error" with the text "Error while connecting to <name>, with id <id>" is shown

#### Scenario: Drivers not active are counted as failing
- **WHEN** a driver reports a state that does not contain "active"
- **THEN** the driver id is added to the list of drivers with errors; it is removed again when the driver reports an active state

#### Scenario: A driver is connecting
- **WHEN** any known driver is in a state containing "connecting"
- **THEN** the status bar shows the connecting indicator (outside the onboarding), tapping it opens the connection status screen
- **AND** the indicator disappears once no driver is connecting anymore

#### Scenario: A driver is deleted
- **WHEN** the core reports a driver as deleted
- **THEN** the driver is removed from the driver list
- **AND** the integration configured with that driver, if any, is removed from the integration list as well

### Requirement: Integration details popup
Tapping an integration (or DPAD_MIDDLE on the selected list entry) SHALL open a full-screen details popup showing the icon, name, whether it is an "External integration" or a "Local integration", a "Manage entities" card with the number of configured entities, the connected switch with the label "Connected" / "Disconnected", and the fields State, Enabled, Id, Version, Developer, Website (if any) and the driver description (if any). A "Delete integration" drawer SHALL sit at the bottom.

#### Scenario: Keypad walk of the details
- **WHEN** the popup opens by keypad
- **THEN** the selection starts on the "Manage entities" card
- **AND** DPAD_DOWN moves to the connected switch and then to the delete opener; DPAD_UP walks back; the focused control is scrolled into view
- **AND** BACK or HOME closes the popup

#### Scenario: Closing the popup
- **WHEN** the popup is closed
- **THEN** it scrolls back to the top, closes the delete drawer and forgets the selected control, so a reopened popup starts on its first control again

### Requirement: Manage entities of an integration
The "Manage entities" card SHALL open a full-screen entity manager with two tabs, "Available: N" and "Configured: N", each showing an entity list with a filter button, Select all / Clear and a footer action. The available tab's action adds the selected entities to the configuration; the configured tab's action is labelled "Remove" and deletes the selected entities. After a change the entity manager closes and the configured entities are reloaded 300–500 ms later.

#### Scenario: Confirming without a selection
- **WHEN** the user triggers Add or Remove with no entity selected
- **THEN** the notification "Select entities" with "Please select entities to add (remove) by tapping in the list." is shown and the manager stays open

#### Scenario: Keypad walk of the entity manager
- **WHEN** the entity manager is open
- **THEN** DPAD_UP / DPAD_DOWN move the selection in the current list, DPAD_MIDDLE toggles the selected row or activates the selected footer control
- **AND** DPAD_LEFT / DPAD_RIGHT move between the footer controls while the selection is on them, and switch between the two tabs otherwise
- **AND** BACK or HOME closes the manager and returns to the details popup

### Requirement: Delete integration with a deliberate second step
The delete drawer in the integration details SHALL ask "Are you sure you want to delete the <name> integration?" with Cancel and Delete. Opened by keypad, the selection SHALL start on Cancel; DPAD_LEFT / DPAD_RIGHT move between Cancel and Delete, DPAD_MIDDLE activates the selected one, BACK and HOME close the drawer. While the drawer is open the page behind it is dimmed and cannot be scrolled.

#### Scenario: Deleting a local integration
- **WHEN** Delete is confirmed for an integration whose driver is not external
- **THEN** a delete request for the integration is sent and the details popup closes

#### Scenario: Deleting an external integration
- **WHEN** Delete is confirmed for an integration whose driver is external
- **THEN** a delete request for the whole integration driver is sent and the details popup closes

#### Scenario: Deletion fails
- **WHEN** the core rejects the deletion
- **THEN** the notification "Error while deleting integration" or "Error while deleting integration driver" is shown

### Requirement: Driver discovery in the "Add an integration" sheet
Opening the "Add an integration" bottom sheet SHALL start driver discovery: the driver list is reloaded first, then a discovery with a 30 s timeout for new drivers only is requested. The discovered-driver list SHALL be cleared, then pre-filled with every already installed driver that has no instance yet, and every driver reported by an `integration_driver_discovered` event SHALL be appended with its name, developer, icon, external marker and setup schema. The header reads "Discovering" with a spinner while the discovery runs and "N integration(s) found" once it stops; tapping the header restarts the discovery. A footer states "Integrations may require the Web Configurator for setup." Closing the sheet SHALL stop the discovery.

#### Scenario: Discovery fails to start or stop
- **WHEN** the core rejects the start or stop request
- **THEN** the notification "Integration discovery failed to start" or "Integration discovery failed to stop" is shown

#### Scenario: Keypad walk of the discovery list
- **WHEN** the first result arrives
- **THEN** it is selected automatically so that DPAD_MIDDLE works without a DPAD_DOWN first
- **AND** DPAD_UP / DPAD_DOWN move the selection, DPAD_MIDDLE selects the driver for setup, BACK / HOME stop the discovery

#### Scenario: Language changes while drivers are listed
- **WHEN** the UI language changes
- **THEN** the names, descriptions and setup-page texts of configured, installed and discovered drivers and of the driver being set up are re-resolved in the new language

### Requirement: Selecting a driver opens the setup popup
Selecting a discovered driver SHALL stop the discovery, mark the driver as the one to set up and open the full-screen "Integration setup" popup. The popup header shows the driver's icon, name and "By <developer>", with a globe icon for an external driver. If the driver ships a setup schema with a title, that schema is the first configuration page; a driver without a setup page starts the setup with an empty form.

#### Scenario: Selected driver is an external, discovered driver
- **WHEN** the selected driver is marked as external
- **THEN** the UI registers it with the core through `configure_discovered_integration_driver` using the driver URL and an empty token before the form is shown
- **AND** on success the driver's settings page from the registration response becomes the first configuration page
- **AND** on failure the failure page is shown with the core's error message

#### Scenario: Driver already installed
- **WHEN** the selected external driver is already in the installed driver list
- **THEN** no registration request is sent

### Requirement: Setup forms are generated from the driver's settings schema
Every configuration page SHALL be rendered from the driver-supplied schema: each setting has an id, a translated label and one field of type text, password, number, textarea, checkbox, dropdown or label. Text, password, number and textarea fields are pre-filled with the schema value (shown as placeholder), a checkbox with its boolean value, a dropdown with its translated items; a label only shows text and takes no input. Labels, dropdown items and page titles SHALL be resolved in the UI language with English as fallback. When a page is submitted, every input field's value is sent as a string keyed by its setting id.

#### Scenario: Dropdown selection
- **WHEN** the user taps a dropdown or presses DPAD_MIDDLE on it
- **THEN** a full-screen selection list with the field label as title opens, with a search field when it has more than 8 items, and the current item preselected
- **AND** selecting an item sets the field value and closes the list; BACK closes the list without a change

#### Scenario: Keypad walk of a form
- **WHEN** a configuration page is shown
- **THEN** the selection starts on the page's first input field; DPAD_DOWN / DPAD_UP walk the fields in schema order, the last field leads to Next, DPAD_LEFT from Next reaches Cancel
- **AND** the on-screen keyboard follows the focused text field and keeps it visible above the keyboard; a focused checkbox or dropdown hides the keyboard
- **AND** Return on a text field moves to the next control

#### Scenario: Confirmation page from the driver
- **WHEN** the driver sends a confirmation page (title, message, optional image, second message)
- **THEN** the page shows the markdown messages and the base64 image; DPAD_DOWN / DPAD_UP scroll the text by 200 px, and DPAD_DOWN at the end moves on to Next

### Requirement: Setup session state machine
Pressing Next on the first page SHALL start the setup with `setup_integration`, sending the driver id, the driver name, the form values and the UI language; from then on the session belongs to this UI. Next on a later settings page SHALL send the values with `set_integration_user_data` (input values), and Next on a confirmation page SHALL send the confirmation. Every `integration_setup_change` event of the session SHALL move the flow: state `SETUP` with a new page or `WAIT_USER_ACTION` shows the next page; `SETUP` without a page shows the cancellable waiting screen; `OK` shows the finish step; `ERROR` shows the failure page with the error text.

#### Scenario: Driver is working
- **WHEN** the event state is `SETUP` and no user action is required
- **THEN** the waiting screen is shown with no time limit; it stays until the next event

#### Scenario: Next page
- **WHEN** an event carries a settings page or a confirmation page that differs from the current one
- **THEN** the page is appended and shown, and the waiting screen is hidden

#### Scenario: Setup finished
- **WHEN** an event with state `OK` arrives
- **THEN** the session ends on the UI side, the configuration pages are dropped, the success animation is played and the add-entities step opens
- **AND** the "Add an integration" sheet closes behind the popup

#### Scenario: Setup failed
- **WHEN** an event with state `ERROR` arrives and this UI did not request the stop
- **THEN** the failure page "Oops – Something went wrong while setting up the integration." shows the error text and a "Try again" button
- **AND** the configuration steps are reset to the first page for the retry

#### Scenario: Start response already reports an error
- **WHEN** the response to the setup start, the user data or the confirmation reports state `ERROR`
- **THEN** the notification "Integration setup error. Aborting setup" is shown and the setup is stopped

### Requirement: Setup error texts
The failure page SHALL show the driver-provided error message in the UI language when the event carries one (English and any language as fallback). Without a driver message the error code is translated: authorization error, connection refused, not found, timeout, "The integration driver is not available", "Invalid input", "The setup has been aborted", "The integration is already configured", "Not supported by the integration driver", otherwise "Unknown error".

#### Scenario: Driver message available
- **WHEN** the error event carries an `error_message` map with an entry for the UI language
- **THEN** that text is shown instead of the generic text for the error code

### Requirement: Rejected input keeps the setup on the same page
When the driver rejects the entered data (error `INVALID_INPUT` with a state other than `ERROR`), the UI SHALL show the current page again with the rejection reason in red below the form instead of ending the setup, so the input can be corrected and submitted again.

#### Scenario: Rejection event followed by the HTTP error of the request
- **WHEN** the rejection has already been shown on the page and the request is then answered with code 400 without a field reference
- **THEN** no additional "Invalid data" notification is shown

#### Scenario: Field-specific rejection
- **WHEN** a request is answered with code 400 and the message quotes a setting id
- **THEN** the error is shown on that field and the keypad selection jumps to it

### Requirement: Setup request errors are reported
When the core rejects a setup start, a user-data submission or a confirmation, the UI SHALL show: for 404 "The integration driver id does not exist."; for 409 an actionable warning "Failed to start setup – There is already a running setup for this integration. Would you like to stop that?" with a "Stop" action that stops the other session; for 422 "The integration is already configured or doesn't allow to be set up again."; for any other code "Cannot start integration setup". The waiting screen SHALL be hidden.

#### Scenario: Setup start rejected
- **WHEN** the start request fails
- **THEN** the session is ended on the UI side and the popup remains on the first page

### Requirement: Setup session keep-alive
When a setup response or event carries a `keepalive_timeout_sec` greater than 0, the UI SHALL renew the session with `integration_setup_keepalive` every third of the lease (at least every 1 s), and renew it immediately after a reconnect to the core or a return to the normal power mode. The keep-alive SHALL stop when the session ends, when the UI cancels it, or when a renewal is answered with 404. A response of a session that is not the one started by this UI SHALL not (re)start the keep-alive.

#### Scenario: Lease of 60 s
- **WHEN** the core reports a keep-alive timeout of 60 s
- **THEN** a keep-alive request is sent every 20 s while the session runs

#### Scenario: Older core without keep-alive
- **WHEN** the core does not report a keep-alive timeout
- **THEN** no keep-alive is sent and the setup behaves as with earlier core versions

### Requirement: Battery time budget of a setup
When the core reports an active setup limit, the setup popup SHALL show a banner counting down the remaining time locally in m:ss, "Running on battery: the setup ends in %1" in orange, or "Low battery: the setup ends in %1" in red. The banner SHALL turn red when less than 60 s remain. The countdown is re-synchronised from every response or event that carries the limit fields, and the banner disappears when the session ends.

#### Scenario: Charger removed during a setup
- **WHEN** an event arrives with the limit active and the same page as before
- **THEN** only the banner is updated; the page is not added a second time

### Requirement: Repeated page events are deduplicated
An event whose `require_user_action` equals the one of the last appended page SHALL not append the page again; without an input error it is ignored, with an input error it re-shows the current page with the error text.

#### Scenario: Same page twice
- **WHEN** two consecutive events carry the same user action
- **THEN** the setup stays on the page shown for the first event

### Requirement: Events of other clients are ignored
`integration_setup_change` events whose driver id is not the session started by this UI (e.g. a setup running in the web configurator) SHALL not affect the setup popup.

#### Scenario: Web configurator runs a setup
- **WHEN** a setup event for another driver arrives while no setup or a different setup runs on the remote
- **THEN** nothing changes on the remote's screen

### Requirement: Cancelling a setup
Cancel, BACK on the configuration step, or BACK on the waiting screen SHALL stop the session with `stop_integration_setup` when one is active, discard the configuration pages, hide the waiting screen and close the setup popup. HOME does the same and returns to the home screen. A stop requested by this UI SHALL NOT show the failure page for the resulting `ERROR` / aborted event.

#### Scenario: Cancel on the waiting screen
- **WHEN** the driver has been working for more than 3 s
- **THEN** the waiting screen shows a Cancel button and BACK cancels as well; before 3 s the wait cannot be cancelled by touch

#### Scenario: Stop request fails
- **WHEN** the core rejects the stop request
- **THEN** the notification "Cannot stop the integration setup" is shown

#### Scenario: Waiting screen time-out
- **WHEN** a waiting screen has been shown for 180 s without a result
- **THEN** the waiting screen closes on its own; the session is not stopped

### Requirement: Add entities after a successful setup
After a successful setup the popup SHALL show "Select entities to control with the remote" with the list of entities the new integration offers, once the core has reported the integration as added. Adding the selected entities configures them and moves to the finish step. The X icon in the header, BACK or HOME skip this step and go to the finish step.

#### Scenario: Add without a selection
- **WHEN** the user triggers Add with no entity selected
- **THEN** the notification "Select entities" is shown and the step stays open

### Requirement: Finish step of a setup
The finish step SHALL show either "You're all set – The integration has been added successfully." with the driver icon, name, Version, Developer and Website and a Done button, or the failure page with a "Try again" button. Done SHALL close the popup; Try again SHALL return to the first configuration page with the popup open. BACK acts as Done on success and as Try again on failure.

#### Scenario: Keypad focus on the finish step
- **WHEN** the finish step is shown
- **THEN** the selection is on Done (success) or on Try again (failure)

### Requirement: Integrations settings page keypad walk
On the integrations settings page DPAD_UP / DPAD_DOWN SHALL move the selection through the integration list; DPAD_DOWN past the last entry selects the "Add an integration" sheet and DPAD_UP from there returns to the list. DPAD_MIDDLE opens the selected integration's details or the sheet. When an integration below the selection is removed, the selection stays on the last entry. Inside the setup popup BACK cancels the running setup instead of only closing the popup.

#### Scenario: Empty list
- **WHEN** no integration is configured and DPAD_MIDDLE is pressed with the list selected
- **THEN** nothing happens

### Requirement: Start and stop of an integration driver
The UI SHALL be able to send a start or stop command for an integration driver; a rejected command SHALL show "Error while starting integration driver" (the same text is used for a failed stop).

#### Scenario: Driver command rejected
- **WHEN** the core rejects a driver start or stop
- **THEN** the notification is shown and the driver state is unchanged until the next state event
