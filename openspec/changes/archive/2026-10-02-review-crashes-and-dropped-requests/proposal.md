## Why

A code review of all C++ sources, looking for the defect classes of the recent entity fixes (a lookup
that can return null, a pointer that outlives its object, a value read before it is assigned), found
two groups of defects. The first group took the app down or sent values that were never set: a group
event during the reload of the groups after a reconnect, a language change after a driver had been
selected for setup, an entity removed while a response or a short timer for it was still due, an
unknown repeat mode of a media player and a setup setting without a field. The second group lost
requests and events without a trace: the discovered-driver setup left out the driver URL, a driver
change event applied nothing, the last entity of a group could not be removed, and the dock
description was read from a misspelt key. Both are fixed on `main` in commit `8d42d46f` and commit
`c410c12b`; this change records the behaviour the living specs do not yet describe.

## What Changes

- A group change, a group deletion or the answer to a group update for a group the UI does not hold
  — typically in the window after a reconnect before the group load has answered — is ignored and
  logged instead of crashing the app. The group load that follows delivers the current group.
- Removing the last entity of a group is saved: "Done" in the group editor always sends the entity
  list, an empty one included, while a rename sends only the name and leaves the entities alone.
- The `repeat` attribute of a media player is validated: OFF, ONE and ALL are accepted in any letter
  case, any other value is ignored and the current mode kept. The repeat command always carries one
  of the three modes.
- A setting of a driver's setup page without a `field` object is skipped like a setting with an
  unknown field type, instead of being shown as an arbitrary input field.
- Starting a driver discovery forgets the driver selected for setup, so a later language change no
  longer touches a driver that was deleted with the previous discovery list.
- Registering a discovered driver sends the driver URL as `driver_url` at the top level of
  `msg_data` of `configure_discovered_integration_driver` (and a token as `token` beside it when one
  is given); before, both were built into an object that was never sent.
- A driver change event from the core is applied at once — name, version, icon, description and the
  other details, and the state through the same path as a driver state event (lower case, list of
  drivers in error) — instead of only after the next reconnect.
- An entity removed while the answer to a media browse or search request, or the 500 ms refresh of
  its translated state text, is still outstanding no longer crashes the app; the late work is
  dropped.
- Not specified here, recorded in `design.md` only: the dock description is read from
  `description` (the UI holds it but shows it nowhere); the unused discovered-driver metadata request
  sends its fields at the top level as well; the activity-bar fix of `Controller::onActivity` was
  replaced by `updatePageActivities()` in commit `d47a9e0e`, which another change records.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `groups`: core-driven group changes for a group that is not loaded, saving an empty entity list,
  a rename that leaves the entities alone.
- `media-player`: validation of the `repeat` attribute and the value the repeat command sends.
- `integrations`: driver change events, the registration request of a discovered driver, the driver
  selection across discoveries, setup settings without a field.
- `entity-management`: late responses and timers of a removed entity.

## Impact

- **Hardware models:** both, Remote Two and Remote 3; nothing here is model-specific.
- **remote-core / Core-API:** no new message. `configure_discovered_integration_driver` now carries
  `driver_url` (and `token` when non-empty) at the top level of `msg_data`, which is where the core's
  request struct reads them and where the Core-API documents them; `update_group` now carries an
  empty `entities` array when the user removed every entity. The existing `integration_driver_change` event,
  the group events of `profile_change` (`group_id` present) and the entity attribute events are consumed as before. UI and core ship together, so no
  fallback is needed (ADR 0005).
- **Third-party code:** none added.
- **Code (already merged):** `src/ui/group/groupController.{h,cpp}`, `src/core/core.{h,cpp}`,
  `src/integration/integrationController.{h,cpp}`, `src/integration/setupSchema.cpp`,
  `src/ui/entity/entityController.cpp`, `src/ui/entity/mediaPlayer.cpp`, the twelve entity classes
  with a language timer, `src/ui/uiController.cpp`; new test targets `testGroupController` and
  `testSetupSchema`, a repeat case in `testEntityController`; `CHANGELOG.md`.
- **Status:** merged on `main` as commit `8d42d46f` and commit `c410c12b`. This change carries the
  behaviour delta only and is archived after reconciliation with the living specs.
