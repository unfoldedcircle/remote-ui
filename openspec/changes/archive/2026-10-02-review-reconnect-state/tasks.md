The implementation is merged in commits `8e97731e` and `2fae27c1`. This change carries the spec
delta only; it is archived on creation.

## 1. Implementation (merged in commits `8e97731e` and `2fae27c1`)

- [x] 1.1 `Power`: the mode starts as Normal and `powerModeChanged` is emitted from one setter, only
      on a change, for the answer after a connect and the event alike (`8e97731e`)
- [x] 1.2 `Battery` reads level, charging state and power supply from the answer to `Power`'s
      `get_power_mode` instead of sending its own request (`2fae27c1`)
- [x] 1.3 `Activity` remembers the last state the core reported, without Unavailable and Unknown,
      and emits `startedExternally` only for a transition into On (`8e97731e`)
- [x] 1.4 `Controller::loadPages()` and `GroupController::setProfileId()` apply only the newest
      answer; the group load uses the profile it was sent for; a group that arrives twice replaces its
      first object (`8e97731e`)
- [x] 1.5 `IntegrationController`: the driver load is counted per page, settles with or without
      success, ignores answers of an earlier load, loads the following pages, resets the status paging
      state; a waiting discovery start is replaced (`8e97731e`)
- [x] 1.6 `DockController`: the dock list is reconciled over all pages after every connect
      (`8e97731e`)
- [x] 1.7 Unit test `testEntityController::activity_onAgainAfterReconnect_isNotStartedExternally`
      (`8e97731e`); no new file, so nothing to register in `remote-ui.pro`, a `.qrc` or a test
      CMakeLists
- [x] 1.8 `CHANGELOG.md`: five entries under `### Fixed` (`8e97731e`)
- [x] 1.9 Unit tests, desktop build and `./cpplint.sh` green; simulator run against the core
      simulator; core restarts on a Remote 3 (`8e97731e`)

## 2. Spec sync (this change)

- [x] 2.1 `power-and-battery`: MODIFIED "Power modes" and "Charging screen" (the "Reboot on charger"
      scenario corrected: the charging screen appears at start-up through the power supply)
- [x] 2.2 `activities`: MODIFIED "Opening activities started outside the remote"
- [x] 2.3 `pages`: MODIFIED "Pages belong to the current profile"
- [x] 2.4 `groups`: MODIFIED "Group definition and loading"
- [x] 2.5 `profiles`: MODIFIED "Current profile is determined by the core"
- [x] 2.6 `core-connection`: MODIFIED "State reset on disconnect and reload on reconnect"
- [x] 2.7 `integrations`: MODIFIED "Integration and driver lists are loaded from the core"
- [x] 2.8 `docks`: MODIFIED "Configured docks are loaded from the core"
- [x] 2.9 `design.md` Current State Analysis against `7eaab4f1`, `adr.md` review manifest (no new
      ADR)
- [x] 2.10 `openspec validate review-reconnect-state --strict` green
- [x] 2.11 Archive this change, which merges the deltas into `openspec/specs/`

## 3. Outstanding device checks

- [ ] 3.1 Restart the core on a device lying on the charger: the charging screen does not open and
      no BatteryCharge sound plays
- [ ] 3.2 Start the UI (reboot) on the charger: the charging screen appears and the BatteryCharge
      sound plays exactly once
- [ ] 3.3 Wake the remote from Suspend on the charger: the charging screen is shown as before
- [ ] 3.4 Restart the core with an activity running and "Open activities started with the API" on,
      on a Remote Two as well: the activity stays unannounced and no screen opens
- [ ] 3.5 Start an activity from the web-configurator while the core is restarting: it is announced
      after the reconnect
- [ ] 3.6 Switch the profile and restart the core right after: pages and groups of one profile,
      each shown once
- [ ] 3.7 Delete or rename a dock while the core is down (or from the web-configurator just before
      a core restart): the dock list follows after the reconnect
