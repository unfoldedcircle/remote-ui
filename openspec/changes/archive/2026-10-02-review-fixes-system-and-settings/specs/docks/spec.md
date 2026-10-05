## MODIFIED Requirements

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
