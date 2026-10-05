# core-connection Specification

## Purpose

How the UI talks to remote-core over the WebSocket Core-API: endpoint and token, authentication, request and response correlation with timeouts, reconnection, the event stream, warnings and the connection state shown to the user.

## Requirements

### Requirement: Core endpoint from the environment
The UI SHALL connect to remote-core over a single WebSocket whose URL is taken from `UC_SOCKET_URL`, defaulting to `ws://127.0.0.1:8080/ws` when the variable is unset or empty. The connection attempt SHALL start immediately at application start, before the UI is shown. The UI SHALL NOT use any other transport (no REST, no second socket).

#### Scenario: Default URL
- **WHEN** the app starts without `UC_SOCKET_URL`
- **THEN** it opens a WebSocket to `ws://127.0.0.1:8080/ws`

#### Scenario: Custom URL
- **WHEN** `UC_SOCKET_URL` is set, e.g. to a Core Simulator on another host
- **THEN** the WebSocket is opened to that URL and no default is used

### Requirement: Authentication handshake
After the socket is open the core sends an `auth_required` event. The UI SHALL answer it with an `auth` request carrying the token read from the file named by `UC_TOKEN_PATH` (whole file content, whitespace trimmed). A `200` response SHALL mark the connection as authenticated ("connected"); any other code SHALL show the warning notification "Authentication to core failed" and leave the connection unauthenticated. `auth` is the only request allowed before authentication.

#### Scenario: Successful authentication
- **WHEN** the core answers the `auth` request with code 200
- **THEN** the UI is connected, input blocking from the startup screen is lifted and all connect-time loading (configuration, API access, active profile, entities, version, update check) starts

#### Scenario: Token rejected
- **WHEN** the core answers `auth` with a code other than 200
- **THEN** the warning notification "Authentication to core failed" is shown
- **AND** the UI stays unauthenticated; no further request is sent until the core sends `auth_required` again

#### Scenario: Token file missing
- **WHEN** `UC_TOKEN_PATH` is unset or the file cannot be read
- **THEN** no `auth` request is sent, a warning is logged and the UI remains unauthenticated (no automatic retry)

### Requirement: Requests are refused while unauthenticated
Every request other than `auth` SHALL be refused locally (request id -1) while the connection is not authenticated, and its response handler SHALL never be registered. Callers therefore see neither a response nor a timeout for such a request.

#### Scenario: Request before authentication
- **WHEN** a component tries to send e.g. `get_entities` before the `auth` response arrived
- **THEN** nothing is sent over the socket and the caller receives -1

#### Scenario: Entity command while disconnected
- **WHEN** the user triggers an entity command while the UI is not connected
- **THEN** the command fails immediately with code 503 "Not connected to the core" and no request is sent

### Requirement: Request/response correlation and timeout
Each request SHALL be a JSON message `{"kind":"req","id":<n>,"msg":<name>,"msg_data":{…}}` with `id` incremented by 1 per request, starting at 1 for the process lifetime. A response is a message with `"kind":"resp"` and `req_id` equal to the request id; codes 200 and 201 are success. For every sent request the UI SHALL start a timer of `UC_UI_REQUEST_TIMEOUT` milliseconds (default 10000 when unset or 0); when it fires before any response the request is settled locally with code 408 "Request timed out". A response arriving after the timeout SHALL be ignored by the handler.

#### Scenario: Normal response
- **WHEN** a response with the matching `req_id` arrives within the timeout
- **THEN** the timeout timer is cancelled and the handler runs once with the response code and payload

#### Scenario: Timeout
- **WHEN** no response arrives within 10 s (default)
- **THEN** the handler receives code 408 with message "Request timed out"
- **AND** a real response arriving later for the same id is dropped

#### Scenario: Custom timeout
- **WHEN** `UC_UI_REQUEST_TIMEOUT=30000` is set
- **THEN** requests wait 30 s before timing out

### Requirement: Response handlers are one-shot
A response handler registered for a request id SHALL run at most once, for the first message that settles the request (a success response, an error response, or the 408 timeout), and SHALL be released afterwards so it does not accumulate for the lifetime of the process. Handlers for a request that was never sent (id -1) SHALL NOT be registered at all. A generic `result` response (e.g. an error or timeout) SHALL settle a request even when the request normally expects a typed response.

#### Scenario: Duplicate response
- **WHEN** two responses arrive for the same request id
- **THEN** only the first one triggers the callbacks

#### Scenario: Error on a typed request
- **WHEN** a request that expects e.g. a `profile` response is answered with a `result` error
- **THEN** the failure callback runs with that code and message and the handler is released

#### Scenario: Unsent request
- **WHEN** a request could not be sent (not connected or socket write failed)
- **THEN** no handler is registered and no callback ever fires for it

### Requirement: Reconnect after connection loss
The connection to the core SHALL stay up for the life of the process, through standby, suspend and wake-up of the remote included: a power mode change SHALL NOT close, reopen or re-authenticate the socket. The connection is lost only when the core itself restarts or crashes, and the first connection after the app starts runs through the same path. When the socket leaves the connected state the UI SHALL emit a disconnect, mark itself unauthenticated and retry the connection every 2000 ms without an upper bound. On a socket error the socket SHALL be closed and the same retry loop started. When the 10th consecutive retry is started (about 20 s after the loss) the UI SHALL raise a connection problem and show the actionable warning "Connection error" / "There was an error connecting to the core. If the issue persists, restart the remote." exactly once per outage. A successful connection resets the retry counter. The UI SHALL NOT send WebSocket ping keep-alives.

#### Scenario: Core restarts
- **WHEN** the core closes the connection and comes back within a few seconds
- **THEN** the UI reconnects on one of the 2 s retries, authenticates again and reloads its state without a notification

#### Scenario: Remote suspends and wakes up
- **WHEN** the remote goes to standby, suspends and wakes up again
- **THEN** the socket stays connected throughout: no disconnect is emitted, no reconnect is started and nothing is reloaded; only the power mode events arrive

#### Scenario: Core unreachable for a long time
- **WHEN** 10 retries have failed
- **THEN** the "Connection error" notification is shown once and the status bar switches to the disconnected indicator
- **AND** retries continue every 2 s until a connection succeeds

### Requirement: Connection state visible in the UI
The UI SHALL expose a "core connected" state that becomes true on successful authentication and false only when the connection problem is raised (after 10 failed reconnects), not on the first loss of the socket. While not connected the status bar SHALL show a red dot. Activity readiness waits SHALL stop waiting as soon as the state is false.

#### Scenario: Short outage
- **WHEN** the socket drops and reconnects within 20 s
- **THEN** the red dot never appears

#### Scenario: Long outage
- **WHEN** the connection problem is raised
- **THEN** the red dot is shown until the next successful authentication

### Requirement: Integration connection indicator
The status bar SHALL show a spinning indicator while any integration is connecting (not while the core itself is reconnecting), except during onboarding. Tapping it SHALL open the "Connection status" popup listing every integration driver in an error state with its state text, or "No connection errors" when the list is empty. BACK, HOME, the close icon or a tap outside close the popup.

#### Scenario: Integration reconnecting
- **WHEN** an integration reports it is connecting
- **THEN** the indicator spins and tapping it opens the list

#### Scenario: Integration error
- **WHEN** an integration fails to connect
- **THEN** the actionable warning "<name> error" / "Error while connecting to <name>, with id <id>" is shown (not during onboarding)

### Requirement: State reset on disconnect and reload on reconnect
On socket loss the UI SHALL mark all entities unavailable, clear pending entity commands (so no loading indicator keeps spinning and a repeated command is not refused as a duplicate) and clear the activity list. On every successful authentication the UI SHALL reload the configuration, the API-access state, the active profile, all entities (pages of 100), the version information and run a non-forced software update check; profile and pages follow from the active profile. The power mode with the battery state, the integration drivers with the integration status, the integrations and the docks SHALL be reloaded on every connect as well, as described in `power-and-battery`, `integrations` and `docks`. A reload SHALL bring the UI to the state the core reports, not add to the state from before the loss: state the core still reports unchanged is not announced as a change, and when a reload overlaps another load of the same list only the newest answer is applied.

#### Scenario: Reconnect after outage
- **WHEN** the UI authenticates again after a loss
- **THEN** entities, configuration and the current profile's pages are reloaded and entities deleted meanwhile disappear

#### Scenario: Reconnect with nothing changed in the core
- **WHEN** the UI authenticates again after a core restart and nothing changed in the core meanwhile
- **THEN** the screens show what they showed before the loss: no page, group or dock appears twice, a running activity is not announced as started, and the charging screen does not open

### Requirement: Event stream without subscription
The core pushes events (`"kind":"event"`) on the authenticated connection; the UI SHALL NOT send any subscription request. The UI SHALL process these events: `auth_required`, `warning`, `entity_change`, `wifi_change`, `integration_driver_change`, `integration_change`, `integration_state`, `profile_change`, `configuration_change`, `dock_change`, `dock_state`, `dock_discovery`, `dock_setup_change`, `dock_update_change`, `integration_discovery`, `integration_setup_change`, `software_update`, `power_mode_change`, `battery_status`, `assistant_event`. `ir_learning` and unknown events SHALL be ignored. Messages that are not valid JSON SHALL be dropped with a log entry. `pong` responses SHALL be ignored.

#### Scenario: Unknown event
- **WHEN** the core sends an event name the UI does not know
- **THEN** the message is ignored and the connection stays up

#### Scenario: Integration state event
- **WHEN** an `integration_state` event carries `integration_id`
- **THEN** it is treated as a device state change of that integration, otherwise as a driver state change

### Requirement: Warning events
A `warning` event carries `event` (`LOW_BATTERY`, `OPEN_CASE`, `BATTERY_UNDERVOLT`), a `shutdown` flag and a `message`. For `OPEN_CASE` the UI SHALL show a full-screen, non-dismissable black screen with a red icon, "Do not operate the device disassembled." and "The remote will turn off in N seconds." counting down from 2, and hide the on-screen keyboard. `LOW_BATTERY` and `BATTERY_UNDERVOLT` SHALL produce the actionable warning notifications described in the `power-and-battery` capability. The `shutdown` flag and `message` are only logged; the message text is never shown.

#### Scenario: Case opened
- **WHEN** an `OPEN_CASE` warning arrives
- **THEN** the disassembly screen covers everything and counts down 2, 1, 0

#### Scenario: Battery warning
- **WHEN** a `LOW_BATTERY` or `BATTERY_UNDERVOLT` warning arrives
- **THEN** the corresponding battery warning notification is shown and the core's message text is not displayed

### Requirement: Core-initiated requests
The core MAY send requests (`"kind":"req"`) to the UI. The UI SHALL answer `get_localization_languages` with a `localization_languages` response `{"kind":"resp","req_id":<id>,"msg":"localization_languages","msg_data":{"version":<ui version>,"translations":[{"name":<native language name>,"code":<xx_YY>},…]}}` listing every embedded translation. Any other request from the core SHALL be ignored.

#### Scenario: Core asks for languages
- **WHEN** the core sends `get_localization_languages`
- **THEN** the UI responds with its build version and the list of translation codes and native names

### Requirement: Credentials never logged
When requests are logged the values of the keys `token`, `password`, `pin` and `admin_pin` SHALL be replaced by `<redacted>` at every nesting level.

#### Scenario: Auth request logged
- **WHEN** the `auth` or `switch_profile` request is written to the log
- **THEN** the token / admin PIN value reads `<redacted>`
