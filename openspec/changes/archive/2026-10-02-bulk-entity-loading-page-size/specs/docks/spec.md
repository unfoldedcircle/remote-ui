## MODIFIED Requirements

### Requirement: Configured docks are loaded from the core
The UI SHALL load the list of configured docks from the core whenever the connection is established, 100 docks per page, and keep it up to date from the core's `dock_change` (added / changed / deleted) and `dock_state_change` events. The page count SHALL be the total `count` of the latest answer divided by the page size of the request (100), rounded up, and at least 1; the `limit` field of an answer holds the number of docks in that page and SHALL NOT be used as the page size. The next page SHALL be requested only while the page just received is below the page count and contained at least one dock. A dock carries id, name, custom WebSocket URL, active flag, model, revision, serial, connection type, firmware version, state (`IDLE`, `CONNECTING`, `ACTIVE`, `RECONNECTING`, `ERROR`), learning-active flag, description and LED brightness.

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
