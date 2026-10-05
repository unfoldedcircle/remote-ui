## Why

Two defects reachable from ordinary use left the remote unusable until it was restarted: renaming an
entity that is no longer loaded took the app down with it, and opening an entity the UI has no screen
for left the screen container stuck, after which no entity, no activity and no settings page could be
opened any more.

## What Changes

- Renaming or re-iconing an entity the UI does not hold — one that was never loaded, or one that was
  deleted while its screen was still open — no longer crashes: the request is not sent and the id is
  logged as a warning.
- Opening an entity for which no screen exists closes again instead of leaving an empty screen
  behind. This is the case for the entity types the UI implements no screen for (`voice_assistant`
  and the `unsupported` placeholder a newer core's entity type falls back to) and for a screen file
  that is missing for a device class.
- After such a failed open, the next entity, activity or settings page opens normally again, on the
  entity level as well as on the level above it (an entity opened from an activity's device list).
- The failed source is written to the log so the missing screen can be found.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `entity-management`: renaming and re-iconing an entity the UI does not hold.
- `entity-detail-controls`: opening a control screen that does not exist.

## Impact

- **Hardware models:** both, Remote Two and Remote 3; neither defect is model-specific.
- **remote-core dependency:** none. No Core-API message is added or changed — on the contrary, a
  request the core would refuse is no longer sent. No remote-core version dependency is introduced;
  an entity type a newer core knows and this UI does not is exactly one of the cases handled.
- **Third-party code:** none added.
- **Code:** `src/ui/entity/entityController.cpp`, `src/qml/main.qml`, `test/ui/CMakeLists.txt`,
  `test/ui/test_entity_controller.cpp` (new), `CHANGELOG.md`.
- **Status:** the implementation is **merged** on `main` as commit `f996d3d8`.
  This change carries the behaviour delta only and is archived on creation.
