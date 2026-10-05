The implementation is merged on `main` as commit `f996d3d8`. This change carries
the behaviour delta, which the archive merges into the living specs.

## 1. Implementation (merged in commit `f996d3d8`)

- [x] 1.1 `EntityController::setEntityName()`: look the entity up once, warn with the id and return
      instead of dereferencing a null pointer
- [x] 1.2 `EntityController::setEntityIcon()`: same guard, so the sibling of a rename behaves the
      same for an id that is gone
- [x] 1.3 `main.qml`: handle `Loader.Error` in the second container — log the source and leave the
      state a regular close leaves (`source = ""`, `active = false`, `close()`, activity flag reset)
- [x] 1.4 `main.qml`: handle `Loader.Error` in the third container, which loads an entity of an
      activity's device list over the same type-derived path
- [x] 1.5 Regression test: new `testEntityController` target in `test/ui/CMakeLists.txt`; it dies on
      SIGSEGV when the controller change is reverted
- [x] 1.6 `CHANGELOG.md` entries under `## Unreleased` / `### Fixed`
- [x] 1.7 Build and the full test suite green in CI; headless start to confirm `main.qml` still
      instantiates

## 2. Spec sync (this change)

- [x] 2.1 `entity-management`: MODIFIED "Rename an entity" and "Change the icon of an entity" — an
      entity the UI does not hold sends nothing and is logged
- [x] 2.2 `entity-detail-controls`: MODIFIED "Opening an entity control screen" — a screen that does
      not exist closes again and the UI stays operable
- [x] 2.3 `design.md` Current State Analysis against `f996d3d8` with the merged `file:line`
      references, `adr.md` review manifest (no new ADR)
- [x] 2.4 `openspec validate entity-controller-robustness --strict` green
- [ ] 2.5 Device check named as outstanding by the merge commit: open a voice assistant entity on
      hardware, confirm it closes again and that the next entity, activity and settings page open

## 3. Archive

- [x] 3.1 Archive this change, which merges the deltas into `openspec/specs/`
