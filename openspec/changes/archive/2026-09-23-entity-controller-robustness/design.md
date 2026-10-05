## Context

Current State Analysis, measured on the merge commit `f996d3d8` (`main`, 2026-09-22).

**Defect 1 — a null entity pointer.** `EntityController` holds only the entities the UI has asked
for, and an entity is taken out of the map as soon as the core reports its deletion, while its
screen can still be open. `setEntityName()` read the translated names straight off the lookup result
(`m_entities.value(entityId)->getNameI18n()`), which is a null pointer for every id that is not in
the map. It now looks the entity up once and returns with a warning
(`src/ui/entity/entityController.cpp:288-296`). `setEntityIcon()` never dereferenced anything — it
only passed the id to `updateEntity()` — but it is the sibling of `setEntityName()` and now carries
the same guard (`entityController.cpp:356-360`), so a stale id is reported the same way instead of
producing a request the core refuses. Every other `m_entities.value(...)` in the file was already
guarded, by a preceding `contains()`, by an `if` on the result, or by a `qobject_cast` that tolerates
null. The regression test is `test/ui/test_entity_controller.cpp`; without the fix it dies on
SIGSEGV.

**Defect 2 — a container that never comes back.** Entity screens are addressed by entity type and
device class: `qrc:/components/entities/<type>/deviceclass/<device class>.qml`
(`src/qml/main.qml:70` and `:106`). The entity type enum has thirteen values and
`src/qml/components/entities/` holds eleven directories: `voice_assistant` and `unsupported` have no
screen at all, and an unknown device class misses the file inside an existing directory. For such a
path the `Loader` ends in `Loader.Error` with a null item, while `loadSecondContainer()` has already
set `active = true` and opened the popup. The only thing that ever released the container again was
the `closed()` signal of the loaded item, which does not exist — so the container stayed active and
`loadSecondContainer()` returned early on `if (!containerSecond.loader.active)` for every screen
afterwards. Both container loaders now handle `Loader.Error` (`src/qml/main.qml:463` and `:549`).

## Goals / Non-Goals

**Goals:** neither defect can take the UI out of service; both leave a log line that names the cause.

**Non-Goals:** adding the missing screens (a voice assistant is driven by the microphone button and
has no screen by design); telling the user that an entity has no screen; auditing the remaining
lookups beyond the two that were wrong.

## Decisions

- **D1 — Guard and log, do not send.** A rename or icon change for an entity the UI does not hold is
  a request the core would refuse anyway, so nothing is sent and the id is logged through
  `lcEntityController`. _Alternative rejected:_ sending it and reporting the core's error, which
  would put an error in front of the user for a screen that is already gone.
- **D2 — `setEntityIcon()` gets the same guard although it never crashed.** Two functions of the
  same edit menu behaving differently for the same id is a trap for the next reader.
- **D3 — The error path leaves exactly the state a regular close leaves.** The second container
  unloads the way its `onClosed` handler does (`source = ""`, `active = false`, `close()`,
  `isActivityOpen = false`); the third container calls `close()`, whose exit transition resets the
  loader. _Alternative rejected:_ a placeholder "not supported" screen — more QML, a screen nobody
  can act on, and it would still have to be closed.
- **D4 — The third container is fixed as well**, because an entity opened from an activity's device
  list is loaded with the same type-derived path and has the identical failure mode.

## Risks / Trade-offs

Failure Mode Analysis — the change touches the screen containers, which are part of the input
ownership chain, and the entity command/edit path:

- **[Resetting `source` and `active` from inside `onStatusChanged` recurses or re-triggers the
  loader]** → verified with a standalone QML probe on the same Qt: a missing `qrc:` source goes
  `Loading` → `Error`, and resetting from the handler returns the loader to `Loader.Null` without
  recursion.
- **[The container closes while the layer below has not taken the input back]** → the error path
  runs the same statements as the regular close, which is the path the input ownership stack is
  built around (ADR 0007); no new ownership transition was introduced.
- **[A screen that merely fails to compile now closes silently instead of being noticed]** → both
  handlers log the source that failed, so the QML error and the source are in the log together.
- **[The guard hides a real bug where an id should have been loaded]** → it is a warning, not a
  debug line, and it names the id.

Resource impact: none. No timer, no polling, no image cache, no Qt module; two branches that run
only on a failure path, and one request fewer.

## Migration Plan

No migration, nothing stored. Merged in commit `f996d3d8`; verified by its unit
tests (`testEntityController`, which fails with SIGSEGV when only the controller change is reverted)
and by CI, plus a headless start of the built binary to confirm `main.qml` still instantiates.
**Device check still to be done:** the merge commit states that defect 2 was not reproduced by
tapping an entity in a running UI, because that machine had no device and no display. The check is a
touch and d-pad one on hardware or in the desktop simulator: open a voice assistant entity, confirm
it closes again, and confirm the next entity, activity and settings page still open.

## Open Questions

None. No in-force ADR is put in question by this change.
