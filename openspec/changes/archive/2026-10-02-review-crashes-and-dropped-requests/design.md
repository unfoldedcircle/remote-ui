## Context

Current State Analysis, measured on `feat/openspec` at commit `3768902e` (2026-10-02), which contains
commit `8d42d46f` and commit `c410c12b` from `main`. The state before each fix is given as _Before_.

### Commit `8d42d46f` — crashes on unknown groups and deleted objects, uninitialised values sent

- **Group events for a group that is not loaded.** `GroupController::setProfileId()` empties the
  group map and asks for the groups again (`src/ui/group/groupController.cpp:101-134`); it runs on
  every profile load, so after every reconnect there is a window in which the map is empty.
  `onGroupChanged()` now returns with a warning for an unknown id (`groupController.cpp:167-173`),
  `onGroupDeleted()` uses `take()` and only deletes what it got (`:193-196`), and the `update_group`
  answer only touches a group it still holds (`:60-64`). _Before:_ all three dereferenced
  `m_groups.value(id)` unchecked, so a group changed or deleted from the web-configurator in that
  window crashed the app. Regression test: `test/ui/test_group_controller.cpp`
  (`groupChanged_unknownGroup_doesNotCrash`, `groupDeleted_unknownGroup_doesNotCrash`,
  `groupDeleted_twice_doesNotCrash`), which dies on SIGSEGV without the fix.
- **Driver selected for setup.** `m_integrationDriverToSetup` points into the discovered-driver list,
  which deletes its items when a discovery starts. It is a `QPointer` now
  (`src/integration/integrationController.h:176`) and is reset, with a change notification, at the
  start of a discovery (`src/integration/integrationController.cpp:294-301`). _Before:_ a raw pointer;
  a language change after the sheet had been opened again wrote into the freed driver.
- **Media browse and search answers.** The response handlers have no context object; they hold a
  `QPointer` to the media player and do nothing once it is gone
  (`src/ui/entity/entityController.cpp:616` and `:642`). _Before:_ they captured the raw pointer.
- **Language timers of the entity classes.** Twelve `QTimer::singleShot(500, ...)` that re-translate
  the state text — started by a language change and by the first display of an entity
  (`entityController.cpp:596` calls `onLanguageChanged()`) — are bound to the entity
  (`singleShot(500, this, ...)`, 12 occurrences under `src/ui/entity/`). _Before:_ no context
  object; an entity removed within 500 ms was written to after it was freed. Note that the entity is
  disposed of 100 ms after its removal, so the window was real.
- **Media player repeat.** `MediaPlayer::updateAttribute()` converts the upper-cased value and keeps
  the current mode, with a warning, when the conversion fails
  (`src/ui/entity/mediaPlayer.cpp:967-982`); `repeat()` starts from OFF and maps every mode other
  than OFF and ONE to OFF (`:306-326`); a new player starts with OFF (`:167`). _Before:_ an unknown
  value was stored as -1, the repeat control showed repeat as active (it is lit for "not OFF"), and
  `repeat()` matched no case and sent an unassigned enum value. Regression test:
  `testEntityController::mediaPlayerRepeat_invalidAttribute_keepsModeAndSendsValidCommand`.
- **Setup setting without a field.** `SetupSchema` initialises the type with -1, the value an unknown
  field name converts to, and the `switch` logs and skips it in a `default` branch
  (`src/integration/setupSchema.cpp:111`, `:143-145`). _Before:_ the type was uninitialised for a
  setting without a `field` object, so the page showed whichever field the stack value selected.
  Regression test: `test/core/test_setup_schema.cpp` (`settingWithoutField_isSkipped`,
  `settingWithUnknownField_isSkipped`, `knownFields_areCreated`).

### Commit `c410c12b` — requests and events that were silently dropped

- **Registration of a discovered driver.** `Api::integrationConfigureDiscoveredDriver()` inserts
  `driver_url` and `token` at the top level of `msg_data`, each only when non-empty
  (`src/core/core.cpp:676-695`). The caller passes the driver URL of the discovery and an empty token
  (`src/qml/components/integrations/Setup.qml:50-55`), so the request carries `driver_id`, `name`
  and `driver_url`. _Before:_ both were put into a `connection` map that was never added to the
  request, so the core got neither and the driver could not be configured. The commit checked the
  placement against the core's `IntegrationDriverSetupParam` and the Core-API document.
- **Metadata request of a discovered driver.** `Api::integrationGetDiscoveredDriverMetadata()`
  (`core.cpp:610-628`) is changed the same way, because the core's
  `GetDiscoveredIntgDriverMetadataMsgData` has no `connection` object (the Core-API document shows
  one; the core wins). The request has no caller in the UI — `getDiscoveredDriverMetadata()` is
  `Q_INVOKABLE` (`integrationController.h:89`) but no QML uses it — so it is not specified.
- **Driver change event.** `IntegrationController::onDriverChanged()`
  (`integrationController.cpp:1169-1201`) looks the driver up, returns for an unknown one, applies
  the details, emits `dataChanged` for the model row and hands a known state to
  `onIntegrationDriverStateChanged()` (`:1197`, the handler at `:1264`), which lower-cases it,
  maintains the list of drivers in error and raises the error notification. _Before:_ the guard was
  inverted (`if (!m_integrationDrivers.contains(...))`), so a known driver got nothing; the state
  would have been stored without lower-casing; and `setDiscovered(deviceDiscovery)` overwrote the
  "found by discovery" flag with the driver's device-discovery capability. That flag is only read on
  the discovered-driver list (`Setup.qml:50`), whose pre-filled entries are copies created with
  `false`, so dropping the overwrite has no visible effect today.
- **Dock description.** Both dock response parsers read `description` (`core.cpp:3067`, `:3098`);
  the event parser already did (`:3210`). _Before:_ `descriptions`, so the description was empty
  until the dock was changed. The UI holds the description (`src/dock/configuredDocks.h:27`) but no
  QML shows it, so no behaviour changes on screen; the living `docks` spec already lists the
  description among the data a dock carries. No delta is written for it.
- **Saving an empty entity list.** `GroupController::updateGroup()` takes the entities as an optional
  `QVariant` (`src/ui/group/groupController.h:33`, `groupController.cpp:49-53`); an invalid variant —
  the argument left out, as `GroupRename.qml:35` does — sends no `entities`, any list replaces them
  (`Api::updateGroup()` `core.cpp:442-463`, the `setEntities` flag at `:459`). The editor passes the
  current list (`src/qml/components/group/GroupEdit.qml:43`), the add dialog its selection
  (`GroupAdd.qml:48`). _Before:_ `Api::updateGroup()` left out an empty list, so removing the last
  entity was never saved. Test: `testGroupController::updateGroup_fromQml_entitiesArgumentIsOptional`
  calls both forms through a `QQmlEngine`.
- **Activity bar after an empty page.** `Controller::onActivity()` continued after an empty page
  instead of returning. The function was later replaced by `updatePageActivities()` in commit
  `d47a9e0e`; that behaviour is recorded with that commit, not here.

## Goals / Non-Goals

**Goals:** no event or response for an object the UI no longer holds can take the app down; no
request carries an unassigned value; requests and events the core sends or expects are no longer
lost; the living specs describe the result.

**Non-Goals:** showing the dock description; a caller for the metadata request; changing the
overlapping profile, group, driver or dock loads after a reconnect (commit `8e97731e`, recorded by
another change); a notification for an ignored group event or an invalid repeat value.

## Decisions

- **D1 — Ignore and log an event for a group that is not loaded.** The group load that follows the
  emptied map delivers the group as it is then, so the event carries nothing the UI would miss.
  _Alternative rejected:_ queueing events until the load answers — more state for a window the load
  already covers.
- **D2 — Guard object lifetimes with Qt's own means.** `QPointer` for a pointer into a list another
  path deletes, a context object for `QTimer::singleShot`. Neither changes behaviour while the object
  lives. _Alternative rejected:_ cancelling outstanding requests on deletion — the core API has no
  cancellation and the answer must still settle the request.
- **D3 — An invalid repeat value keeps the current mode; the command defaults to OFF.** The core
  validates the attribute; a value the UI does not know is a defect upstream, and keeping the last
  valid mode is the least surprising display. Accepting lower case follows integrations that send it.
  The OFF default of `repeat()` is defensive only: after D3's validation the mode can no longer be
  outside the three values.
- **D4 — A setting without a field is skipped like an unknown field type.** It is the existing path
  for a field the UI cannot render; the user sees the rest of the page.
- **D5 — The core's request structs decide where fields go.** `driver_url` and `token` are top-level
  fields of `msg_data` for both discovered-driver requests, even where the Core-API document shows a
  `connection` object (ADR 0005: the core is the reference, UI and core ship together).
- **D6 — "Left out" and "empty" are different for a group's entities.** The optional `QVariant` keeps
  the QML call sites as they were: a rename passes no list, the editor and the add dialog pass one.
  _Alternative rejected:_ a separate rename function — a second Core-API path for the same message.
- **D7 — A driver change goes through the state handler.** One place lower-cases the state and keeps
  the list of drivers in error, whichever event delivered the state.

## Risks / Trade-offs

Failure Mode Analysis — the change touches Core-API event and response handling:

- [An ignored group CHANGE event is lost if no group load follows] → the map is only emptied by
  `setProfileId()`, which always sends the load; a group that is not in the map for another reason
  was never shown, and the next profile load brings it.
- [`m_integrationDriverToSetup` becomes null while the setup popup reads it] → it is reset only when
  a discovery starts, which the user triggers from the sheet underneath the full-screen popup; the
  popup reads the driver's `discovered` flag once, when it is created. Not verified on a device.
- [The core rejects an empty `entities` list, or reads it as "no change"] → not established: the
  commit's tests check the request the UI builds, not the core's answer. Device check 1 below; until
  then the editor behaves at worst as before (the group keeps its entities).
- [A driver change event without a state wipes the state] → a state that does not convert (-1) is
  not passed on; the state then stays as the last state event set it.
- [The setup schema replaced by a driver change while a setup runs] → the running setup shows a copy
  of the schema (`selectIntegrationToSetup()`), so the replacement does not affect it.

Resource impact: none. No timer, polling, image cache, animation or Qt module is added; the guards
are a null check each, the timers are the existing ones bound to a context object.

## Migration Plan

No migration, nothing stored. Merged on `main` as commit `8d42d46f` and commit `c410c12b`; archived
after reconciliation with the living specs. Verified by the unit tests named above (`make test`
20/20 at the time of the first commit), the desktop build and `./cpplint.sh`; the field placement of
the driver registration was checked against the core's request structs, the group call forms through
a `QQmlEngine`. Neither commit was verified on a device or end to end against a running core.

Device checks still owed (Remote Two or Remote 3, against a real core; the simulator suffices for
the first three):

1. Remove every entity of a group in the group editor, confirm with Done: the core keeps the empty
   group and the tile shows it as empty after a reconnect.
2. Rename a group with entities: the entities stay.
3. Restart the core while the web-configurator changes and deletes a group on the current page:
   the remote keeps running and shows the group as the reload reports it.
4. Set up a driver found by the discovery (an external driver announced on the network): the
   registration succeeds and the driver's setup page is shown.
5. Update an integration driver to a new version: the integration list shows the new version without
   a reconnect.
6. A media player integration that reports `repeat` in lower case: the control shows the mode and
   cycles correctly.

## Open Questions

- Should the dock description be shown somewhere (dock details)? Today the fix only makes the held
  value correct; the `CHANGELOG.md` entry of commit `c410c12b` says "is shown", which no screen does.
- Should the unused discovered-driver metadata request be removed, or is a caller planned?
- The `integrations` spec scenario named "Selected driver is an external, discovered driver" is
  triggered by the discovery flag, not by the external flag; the heading is kept because a
  `MODIFIED` block keeps its scenario names. Rename it in a later change?
