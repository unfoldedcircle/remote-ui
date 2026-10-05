## Why

The code review found five defects that show when the UI connects to the core again. The
WebSocket to the core stays up through standby, suspend and wake-up; it is lost only when the core
restarts or crashes, and the first connect after the app starts runs through the same code
(`core-connection`, "Reconnect after connection loss"). Every reload that follows such a connect
assumed it was the first and only one: the charging screen opened on the charger after a core
restart, a running activity was announced as started from another device, overlapping page and
group loads stacked their answers, the integration driver load could stay unfinished for good, and
the dock list was only ever added to. A related one-liner asked the core for the power mode twice
per connect. The implementation is **merged** (commit `8e97731e` "state that went wrong after a
reconnect", and commit `2fae27c1` "one power mode request per connect"); this change carries the
spec delta only.

## What Changes

- **Power mode.** It is requested once per connect; the same answer supplies the battery state.
  The mode starts as Normal, and a reported mode equal to the current one is not published, so the
  Normal → Normal of a reconnect and the first answer after the start no longer look like a wake-up.
  The charging screen no longer opens on the charger when the app reconnects. At start-up on the
  charger it still appears, through the power supply status (the battery starts with no power
  supply), and it still appears on a real wake-up on the charger.
- **Activities.** The disconnect forces every entity to Unavailable, and the reload after the
  reconnect looked like a fresh start of a running activity. The activity remembers the last state
  the core reported, ignoring Unavailable and Unknown, and only a transition into On from another
  state counts as a start from outside the remote.
- **Pages and groups.** When page or group loads overlap (a reconnect during a profile switch, a
  resync after a rejected page change), only the answer to the newest request is applied, the
  group load uses the profile it was sent for, and a group that arrives twice replaces its first
  copy.
- **Integration drivers.** The driver load always completes. A failed driver request, an empty
  driver list, a failed or unsendable list request, or more than 100 drivers no longer leave it
  unfinished: it is counted per page, a page settles when all its driver requests are answered with
  or without success, the following pages are loaded with the same page size, answers of an earlier
  load are ignored, and then the integration status load (from page 1) and a waiting discovery start
  run. A discovery start that is still waiting for the driver load is replaced, not added to.
- **Docks.** After a connect the dock list is reconciled with the core's answer: known docks are
  updated, unknown ones added, docks missing from every page removed, and every page is loaded.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `power-and-battery`: "Power modes" (one request per connect, only a change is published, Normal
  at start) and "Charging screen" (not on a reconnect, at start-up through the power supply).
- `activities`: "Opening activities started outside the remote" (a reconnect is not a start).
- `pages`: "Pages belong to the current profile" (only the newest page load counts).
- `groups`: "Group definition and loading" (only the newest group load counts, for its own
  profile; a group arriving twice is replaced).
- `profiles`: "Current profile is determined by the core" (a reconnect during a profile switch).
- `core-connection`: "State reset on disconnect and reload on reconnect" (overlapping reloads, the
  other lists reloaded on connect).
- `integrations`: "Integration and driver lists are loaded from the core" (the load always
  completes).
- `docks`: "Configured docks are loaded from the core" (reconciliation after a connect, paging).

## Impact

- **Hardware models:** both, Remote Two and Remote 3; nothing here is model specific. The charging
  screen exists on both remotes.
- **remote-core dependency:** none added. The change uses the existing `get_power_mode`,
  `power_mode_change`, `battery_status`, `entity_change`, `get_pages`, `get_groups`,
  `get_integration_drivers`, `get_integration_driver`, `get_integration_status`,
  `start_integration_discovery` and `get_docks` messages; UI and core ship together (ADR 0005).
- **Third-party code:** none added.
- **Code (already merged):** `src/system/power.{h,cpp}`, `src/system/battery.{h,cpp}`,
  `src/ui/entity/activity.{h,cpp}`, `src/ui/uiController.{h,cpp}`,
  `src/ui/group/groupController.{h,cpp}`, `src/integration/integrationController.{h,cpp}`,
  `src/dock/dockController.{h,cpp}`, `test/ui/test_entity_controller.cpp` (one new case),
  `CHANGELOG.md` (five entries under `### Fixed`). No new file, so no registration.
- **Status:** implemented and merged in commits `8e97731e` and `2fae27c1`. This change is archived
  on creation, so the deltas land in the living specs immediately.
