## MODIFIED Requirements

### Requirement: State reset on disconnect and reload on reconnect
On socket loss the UI SHALL mark all entities unavailable, clear pending entity commands (so no loading indicator keeps spinning and a repeated command is not refused as a duplicate) and clear the activity list. On every successful authentication the UI SHALL reload the configuration, the API-access state, the active profile, all entities (pages of 100), the version information and run a non-forced software update check; profile and pages follow from the active profile. The power mode with the battery state, the integration drivers with the integration status, the integrations and the docks SHALL be reloaded on every connect as well, as described in `power-and-battery`, `integrations` and `docks`. A reload SHALL bring the UI to the state the core reports, not add to the state from before the loss: state the core still reports unchanged is not announced as a change, and when a reload overlaps another load of the same list only the newest answer is applied.

#### Scenario: Reconnect after outage
- **WHEN** the UI authenticates again after a loss
- **THEN** entities, configuration and the current profile's pages are reloaded and entities deleted meanwhile disappear

#### Scenario: Reconnect with nothing changed in the core
- **WHEN** the UI authenticates again after a core restart and nothing changed in the core meanwhile
- **THEN** the screens show what they showed before the loss: no page, group or dock appears twice, a running activity is not announced as started, and the charging screen does not open
