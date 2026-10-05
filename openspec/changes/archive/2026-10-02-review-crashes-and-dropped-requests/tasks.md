The implementation is merged on `main` as commit `8d42d46f` and commit `c410c12b`. This change
carries the behaviour delta, which the archive merges into the living specs.

## 1. Crashes and uninitialised values (merged in commit `8d42d46f`)

- [x] 1.1 `GroupController`: `onGroupChanged` ignores and logs an unknown group, `onGroupDeleted`
      uses `take()`, the `update_group` answer only touches a group it still holds
- [x] 1.2 `IntegrationController`: the driver selected for setup is a `QPointer`, reset when a
      discovery starts
- [x] 1.3 `EntityController`: the media browse and search answers hold a `QPointer` to the player
- [x] 1.4 Twelve entity classes: the 500 ms language timer is bound to the entity
- [x] 1.5 `MediaPlayer`: `repeat` validated and accepted in lower case, an invalid value keeps the
      mode; `repeat()` defaults to OFF
- [x] 1.6 `SetupSchema`: a setting without a `field` object is skipped and logged
- [x] 1.7 New `testGroupController` (dies on SIGSEGV without 1.1) and `testSetupSchema` targets in
      `test/ui/CMakeLists.txt` and `test/core/CMakeLists.txt`; repeat case in `testEntityController`
- [x] 1.8 `CHANGELOG.md` entries under `### Fixed`

## 2. Dropped requests and events (merged in commit `c410c12b`, depends on 1.1 and 1.7)

- [x] 2.1 `Api::integrationConfigureDiscoveredDriver` and `Api::integrationGetDiscoveredDriverMetadata`:
      `driver_url` and `token` at the top level of `msg_data`
- [x] 2.2 `IntegrationController::onDriverChanged`: guard fixed, details applied, model row notified,
      state through the state handler, `discovered` no longer overwritten
- [x] 2.3 `Controller::onActivity` continues after an empty page (since replaced by
      `updatePageActivities()` in commit `d47a9e0e`, recorded elsewhere)
- [x] 2.4 Dock response parsers read `description`
- [x] 2.5 `GroupController::updateGroup` with an optional `QVariant` entity list,
      `Api::updateGroup` with `setEntities`; both QML call forms in `testGroupController`
- [x] 2.6 `CHANGELOG.md` entries under `### Fixed`
- [x] 2.7 `make test`, `make linux` and `./cpplint.sh` green for both commits

## 3. Tests still missing (ADR 0009)

- [ ] 3.1 Request-builder test: `configure_discovered_integration_driver` carries `driver_url` (and a
      non-empty `token`) at the top level of `msg_data` and no `connection` object
- [ ] 3.2 `onDriverChanged` test: a known driver's name and version change, the state arrives lower
      case and the driver enters the list of drivers in error
- [ ] 3.3 Dock parser test: the description of `get_docks` / `get_dock` is read

## 4. Spec sync (this change)

- [x] 4.1 `groups`: MODIFIED "Editing the entities of a group", "Renaming a group", "Core-driven
      group changes"
- [x] 4.2 `media-player`: MODIFIED "Attributes and options", "Playback and control commands"
- [x] 4.3 `integrations`: MODIFIED "Integration and driver state follow core events", "Selecting a
      driver opens the setup popup", "Setup forms are generated from the driver's settings schema"
- [x] 4.4 `entity-management`: MODIFIED "Entity removal"
- [x] 4.5 `design.md` Current State Analysis against commit `3768902e` with `file:line` references,
      `adr.md` review manifest (no new ADR)
- [x] 4.6 `openspec validate review-crashes-and-dropped-requests --strict` green

## 5. Device checks (neither commit was verified on hardware)

- [ ] 5.1 Remove every entity of a group with Done: the core keeps the group empty after a reconnect
- [ ] 5.2 Rename a group with entities: the entities stay
- [ ] 5.3 Restart the core while another client changes and deletes a group of the current page: the
      remote keeps running and shows the reloaded group
- [ ] 5.4 Set up a driver found by the discovery: the registration succeeds and its setup page shows
- [ ] 5.5 Update an integration driver: the new version shows without a reconnect
- [ ] 5.6 A media player reporting `repeat` in lower case: the control shows and cycles the mode

## 6. Archive

- [x] 6.1 Reconcile with the change that records commit `8e97731e` (both touch `GroupController`
      and the `integrations` capability), then archive this change
