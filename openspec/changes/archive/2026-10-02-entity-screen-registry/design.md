## Context

Current State Analysis, measured on `main` @ `1cc19491`:

- **Screen path built in QML, seven times:** `src/qml/main.qml` (two call sites),
  `src/qml/components/entities/Base.qml` (three), `src/qml/components/Page.qml` (one),
  `src/qml/components/entities/activity/LoadingScreen.qml` (one) and
  `activity/deviceclass/Activity.qml` (one, for the third container) all concatenate
  `"qrc:/components/entities/" + entityObj.getTypeAsString() + "/deviceclass/" + entityObj.getDeviceClass() + ".qml"`.
- **Device class normalised per entity class:** every entity constructor converts the core's
  `device_class` to its own enum (`src/ui/entity/cover.cpp`, `mediaPlayer.cpp`, `sensor.cpp`,
  `switch.cpp`, `climate.cpp`, `button.cpp`, `macro.cpp`, `select.cpp`, `voiceAssistant.cpp`,
  `light.cpp`, `remote.cpp`, `activity.cpp`), falls back to a per-type default (cover `Blind`,
  media player `Speaker`, switch `Switch`, sensor `Custom`, …) and aliases some values (cover
  `Shade` → `Blind`, `Door`/`Gate`/`Window` → `Window`). `sensor.h` warns in a comment that
  changing device classes requires QML screen support.
- **The screens:** `src/qml/components/entities/<type>/deviceclass/*.qml` for eleven types —
  activity, button, climate, cover (Blind, Curtain, Garage, Window), light, macro, media_player
  (Receiver, Set_top_box, Speaker, Streaming_box, Tv), remote, select, sensor (nine classes),
  switch (Switch, Outlet). No directory for `voice_assistant` or the `Unsupported` type.
- **The failure path:** a path with no file is a `Loader.Error`; since commit `f996d3d8`
  `main.qml` releases the container on it and logs the source, so the UI stays usable.
- **What the specs say:** `entity-management` "Device class fallback" lists the defaults and
  aliases; `entity-detail-controls` "Opening an entity control screen" says a type without a
  screen "closes again by itself" — the error path described as behaviour.

## Goals / Non-Goals

**Goals:** one registry for type/device class → screen; QML asks, never builds a path; an
unknown type opens nothing by rule; a test that fails when a registered screen is not in the
resources or a screen file is unreachable.

**Non-Goals:** changing which screen any supported entity gets; changing tiles, icons or the
device class the entity reports; a QML-side component registry or `qmlRegisterType` per screen;
new screens for the voice assistant.

## Decisions

- **D1 — The registry is C++, a static table** (ADR 0012): entity type → screen directory and
  default screen, device class → screen file, aliases. `EntityController` exposes one
  `Q_INVOKABLE` that returns the screen URL for an entity, or an empty URL.
- **D2 — Empty URL means "open nothing".** Voice assistant, unsupported and unknown types return
  no URL; the call sites open nothing and log at debug. The `Loader.Error` handling in `main.qml`
  stays for the remaining case, a registered screen whose file is not embedded.
- **D3 — Aliasing lives in one place; the entity still reports the normalised class.** Nothing
  but the screen lookup reads `getDeviceClass()`, but it is a `Q_INVOKABLE` that reports the
  device class, so its value stays: the cover constructor now asks the registry for the screen
  name (`Blind`, `Window`) instead of holding its own alias switch, and a data-driven test pins
  what a cover reports for every device class. The other constructors keep validating the
  core's value against their enum and falling back to their default, which the registry's
  defaults match — that is validation of the core's value, not a screen choice.
- **D4 — The test parses `resources/qrc/main.qrc`.** `testEntityScreens` resolves every URL the
  registry can produce against the `alias=` entries and the files on disk, checks that the device
  class enum keys of every entity class match the registry's keys exactly (no stale and no
  implicit entries), that unknown classes fall back to the default, that the cover aliases hold,
  and that every file under `components/entities/*/deviceclass/` is reachable from the registry.
  Removing a qrc entry makes it fail with a clear message.

## Risks / Trade-offs

- [A screen exists only for a device class the registry does not list] → the test catches it
  (unreachable file), and the entity falls back to the type's default screen as today.
- [The core sends a device class newer than the UI] → the type's default screen, as today; the
  registry makes the default explicit and tested.
- [Moving the aliasing changes a reported device class] → D3: keep the constructor
  normalisation if any observable value depends on it.

## Migration Plan

Merge the implementation branch; no data, no core change. Archive this change afterwards.
Rollback is reverting the commit.

## Open Questions

None.
