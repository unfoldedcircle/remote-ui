## Why

Which control screen an entity gets is decided in three places that nobody keeps in step by
design: each C++ entity class validates and aliases the core's device class against its own enum,
six QML call sites build the screen path by string concatenation
(`qrc:/components/entities/<type>/deviceclass/<DeviceClass>.qml`), and the file tree under
`src/qml/components/entities/` plus its `main.qrc` registration is the actual truth — checked only
at runtime, where a missing screen surfaces as a loader error that the container has to recover
from. A type without a screen (voice assistant, an unsupported type) is handled by that error path
too. The maintainer decided: one registry in C++, an unknown type shows no detail screen, and a
test that catches a screen missing from the resources before a device does.
**The implementation is on the branch `refactor/entity-screen-registry` (pull request open).**

## What Changes

- **One registry** (`src/ui/entity/entityScreens.*`) maps entity type and device class to the
  screen: the screen directory and default per type, the screen per device class, and the
  aliases that were spread over the entity constructors (cover `shade` → blind, `door`/`gate`
  → window; unknown device class → the type's default). It is the single place a new entity
  type or device class screen is registered.
- **QML asks the registry** through the entity controller (`EntityController.screenUrl(entity)`)
  instead of building a path; the seven string concatenations — a seventh sat in the activity
  screen's third-container loader — are gone (ADR 0012).
- **A type without a screen opens nothing.** The registry answers with no URL for a voice
  assistant, an unsupported type or anything it does not know; nothing is loaded, nothing has to
  close again, and the miss is logged. The loader error handling stays as a safety net for a
  registered screen whose file is missing.
- **A unit test** (`testEntityScreens`) checks that every type and device class resolves as
  specified and that every screen the registry can name is registered in `main.qrc`, so a
  forgotten resource entry fails CI instead of the device.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `entity-management`: the device class fallback becomes the registry's rule.
- `entity-detail-controls`: opening a control screen consults the registry; a type without a
  screen opens nothing instead of a screen that closes again.

## Impact

- **Hardware models:** both, no visible difference for supported entities; a voice assistant or
  unsupported entity no longer flashes a failed screen.
- **remote-core dependency:** none.
- **Code:** `src/ui/entity/entityScreens.{h,cpp}` (new), `EntityController::screenUrl`,
  `main.qml`, `components/entities/Base.qml`, `components/Page.qml`,
  `components/entities/activity/LoadingScreen.qml` and `activity/deviceclass/Activity.qml`,
  `cover.cpp` (its alias switch now asks the registry), `sensor.h` (comment), `remote-ui.pro`,
  `test/ui/CMakeLists.txt` (`testEntityScreens`, plus the registry in the cover and entity
  controller test targets), `test/ui/test_cover_entity.cpp`, `CLAUDE.md`.
