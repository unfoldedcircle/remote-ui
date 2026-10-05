## Context

The implementation is merged. Current State Analysis measured against `7eaab4f1` (`feat/openspec`,
2026-10-02, which contains `main` with the squash commit `8e97731e` "state that went wrong after a
reconnect" — 14 files, +253 / −58 — and the follow-up `2fae27c1` "one power mode request per
connect", 2 files), with the state before the fix in brackets.

**When this code runs.** The WebSocket to the core stays up through standby, suspend and wake-up;
it is lost only when the core restarts or crashes (`core-connection`, "Reconnect after connection
loss"). Every controller below reloads on `core::Api::connected`, and the first connect after the
start of the app is the same signal, so each defect could show at start-up as well as after a core
restart.

- **Power mode.** `src/system/power.h:52` `m_powerMode = Normal` [_uninitialised_];
  `src/system/power.cpp:95` `Power::setPowerMode()` returns when the mode is unchanged and only then
  emits `powerModeChanged(old, new)`; both sources use it — the answer to the request sent on every
  connect (`power.cpp:19`, `:41`) and the `power_mode_change` event (`power.cpp:92`). [_Both paths
  emitted unconditionally, so a reconnect published Normal → Normal and the first answer published a
  transition from uninitialised memory._] The charging-screen decision is in QML,
  `src/qml/main.qml:638` (`toPowerMode === Normal && fromPowerMode !== Idle && Battery.isCharging &&
  Battery.powerSupply`), which the Normal → Normal of a reconnect satisfied on the charger.
- **One request per connect** (`2fae27c1`). `src/system/battery.cpp:22` connects
  `Battery::onPowerModeResponse()` (`battery.cpp:57`) to `core::Api::respPowerMode` and reads the
  battery level, charging state and power supply from the answer to `Power`'s request. [_`Battery`
  sent a second `get_power_mode` of its own on every connect._]
- **Charging screen at start-up.** `src/system/battery.h:53` `m_powerSupply = false`, and
  `Battery::setPowerSupply()` (`battery.cpp:49`) emits `powerSupplyChanged` only on a change, so the
  first answer on the charger emits `powerSupplyChanged(true)`, which opens the charging screen and
  plays BatteryCharge (`main.qml:622`). Checked by reading; this path is unchanged by the fix. On a
  reconnect the power supply is already true, so nothing is emitted.
- **Activity started externally.** `src/ui/entity/activity.h:226` `m_lastReportedState = -1`;
  `src/ui/entity/activity.cpp:182` takes `wasOn` from it before the update and stores every new
  state except Unavailable and Unknown; `activity.cpp:203` emits `startedExternally` only when not
  `wasOn`. The disconnect sets every entity Unavailable (`src/ui/entity/entityController.cpp:517`
  `onCoreDisconnected()` → `setAllEntitiesAvailable(false)`), and `entityController.cpp:568` wires
  `startedExternally` to the "open activity" path. [_A running activity went On → Unavailable → On
  across a core restart and was announced as an external start._] The constructor applies the
  initial attributes before that connection exists, so the first load of a running activity was and
  is not announced.
- **Pages.** `src/ui/uiController.cpp:639` `Controller::loadPages()` stores the request id in
  `m_pagesRequestId` (`uiController.h:235`, `uiController.cpp:646`); the success handler drops a
  stale answer (`:652`) and clears the list before applying the answer, and the failure handler
  ignores a stale failure (`:674`). [_Every answer appended to the same list._]
- **Groups.** `src/ui/group/groupController.cpp:101` `GroupController::setProfileId()` stores
  `m_groupsRequestId` (`groupController.h:59`, `.cpp:113`), drops a stale answer (`:119`) and adds
  the groups under the captured `profileId` (`:126`) [_`m_profileId`, which a later switch had
  already changed_]; `onGroupAdded()` replaces an existing object of the same id (`:150`) [_the
  second object overwrote the hash entry and the first leaked_].
- **Integration drivers.** `src/integration/integrationController.cpp:87`
  `getAllIntegrationDrivers()` bumps `m_driverLoadGeneration` (`.cpp:95`) and resets the pending
  counter; `getIntegrationDrivers()` (`:159`–`:209`) ends the load with `integrationDriversLoaded`
  when the request cannot be sent (`:167`), when a page is empty (`:192`) and when the page request
  fails (`:209`), and ignores answers of an earlier generation (`:176`); every driver request settles
  through `onIntegrationDriverSettled()` (`:1077`) on success and on failure (`:380`, `:401`), which
  loads the next page with `m_integrationDrivers.limit` or ends the load; `onIntegrationDriversLoaded()`
  (`:1096`) starts the status load, and `getAllIntegrationStatus()` (`:78`) resets its paging state
  first. A discovery start that still waits for the driver load is replaced (`:305`,
  `m_discoveryStartScope`). [_`integrationDriversLoaded` fired only when the count of loaded drivers
  equalled the total: one failed driver, an empty list, a failed list request or a second page never
  got there, so neither the status load nor the waiting discovery started, and every further
  discovery start added another waiting receiver that all fired later._]
- **Docks.** `src/dock/dockController.cpp:287` `getDocks()` starts a new generation
  (`m_dockLoadGeneration`, `dockController.h`) with a shared set of loaded ids; `loadDocks()`
  (`:292`) ignores a stale page (`:302`), updates a known dock through `onDockChanged()` plus its
  state, appends an unknown one, loads the next page while it is not the last and not empty, and at
  the end removes every listed dock missing from the set through `onDockDeleted()` (`:342`–`:347`).
  The load runs on every connect (`:389`). [_Only unknown docks were appended, from the first page._]
- **Tests.** `test/ui/test_entity_controller.cpp:272`
  `activity_onAgainAfterReconnect_isNotStartedExternally` covers the activity case, including an
  activity started while the connection was down. The power mode, the page and group guards, the
  driver load and the dock reconciliation have no unit test.

## Goals / Non-Goals

**Goals:** after a connect, the UI shows what the core reports, without announcing as a change what
did not change; every load that others wait for comes to an end.

**Non-Goals:** keeping the connection up across a core restart, changing what is reloaded on a
connect, retrying a failed dock list request, moving the charging-screen decision out of QML,
reworking the entity reload.

## Decisions

- **D1 — `powerModeChanged` is a transition.** Emitted from one setter, only on a change, with
  Normal as the start value: the UI runs only while the remote is awake. _Alternative rejected:_
  suppressing the charging screen in QML on a reconnect flag, which would leave every other
  listener of the signal (window visibility, touch handling) reacting to a non-change.
- **D2 — One `get_power_mode` per connect,** owned by `Power`; `Battery` reads the same answer.
- **D3 — The charging screen at start-up comes from the power supply only.** The power mode path
  is for a wake-up; start-up on the charger is covered by `Battery` starting with no power supply.
- **D4 — An activity remembers the last state the core reported,** ignoring Unavailable (which the
  UI sets itself on a disconnect) and Unknown. _Alternative rejected:_ suppressing
  `startedExternally` for some time after a reconnect, which would also hide a real start made
  while the core was down.
- **D5 — Newest request wins** for pages and groups, by request id; the answer replaces the list.
  _Alternative rejected:_ cancelling the earlier request, which the Core-API client cannot do.
- **D6 — A load generation for the driver and dock lists,** so answers of a superseded load cannot
  touch the new list, and a load that others wait for settles on every outcome.
- **D7 — Reconcile the docks instead of clearing them,** so the list does not flash empty and open
  dock screens keep their objects for docks that still exist.

## Risks / Trade-offs

Failure Mode Analysis — the change sits on the Core-API connection, power modes and activities:

- [A real wake-up whose answer equals the held mode is no longer published] → the wake-up arrives
  as a `power_mode_change` event from Suspend or Low_power, which is a change; only an equal mode is
  dropped, and that never was a transition.
- [The charging screen no longer opens at start-up on the charger] → it does, through
  `powerSupplyChanged`; checked by reading only — see the device checks.
- [The core restarts while the remote is in Idle or Low_power and reports Normal] → published as a
  change, as before; the window and touch handling follow it.
- [An activity started from another client while the core was down is missed] → not missed: Off is
  the last reported state, so On afterwards is a start (unit test).
- [An activity that the core reports as Unknown and then On] → the state before Unknown decides;
  an activity that was On stays unannounced, which matches "it was running all along".
- [A failed page request of the newest load leaves an empty page area] → unchanged behaviour for the
  newest request; a stale failure no longer empties anything.
- [A driver page whose requests never get an answer] → each driver request has the Core-API request
  timeout and its failure settles the page; a request that cannot be sent settles at once.
- [A dock list request fails] → logged only, no reconciliation and no `docksLoaded`; the list keeps
  the docks from before until the next connect (unchanged, not addressed here).
- [A dock is removed by reconciliation while its details popup is open] → it goes through the same
  `onDockDeleted()` as a delete event from the core, so the popup reacts as it does to that event.

Resource impact: negligible. No timer, polling, animation or Qt module is added. A connect sends one
request fewer (`get_power_mode`); stale answers are dropped by an integer comparison; the dock load
holds one `QSet` of dock ids for its duration. Binary size grows by a few hundred bytes against the
100 MB budget; CPU, memory, frame rate and input-to-command latency are untouched.

## Migration Plan

No migration: nothing is stored and no setting changes. Merged in commits `8e97731e` and
`2fae27c1`. The merge commit records: the unit tests (20 of 20, including the new activity case),
the desktop build and `./cpplint.sh`, a simulator run against the core simulator (authentication,
docks, drivers and the integration status load complete, no QML errors), and core restarts on a
Remote 3 with debug logging: no activity reported as started
externally, the pages loaded once, the driver load completes after each restart, the docks
reconciled. The power mode part and the start-up on the charger were checked by reading, not on a
device. The power and battery paths need a real device (ADR 0003, ADR 0016); the remaining checks
are listed in `tasks.md`.

## Open Questions

- The charging-screen decision (`main.qml:638`) and the power-supply reaction (`main.qml:622`) are
  logic in QML, a deviation under ADR 0012. Moving them to C++ with a unit test would also give the
  power mode transition its missing test. Not done here; which change takes it?
- The page and group guards, the driver load and the dock reconciliation have no unit test
  (ADR 0009). They need a fake core answer; is that test harness wanted, or are they accepted as
  verified on the device?
- Should a failed `get_docks` be retried like `get_profiles` (every 2 s), instead of leaving the list
  of the previous connection in place?
