The implementation is merged on `main` as commit `3778fc27`. This change carries
the behaviour delta, which the archive merges into the living specs.

## 1. Implementation (merged in commit `3778fc27`)

- [x] 1.1 Stale-response guard in the shared model `uc::ui::Entities`: remember the newest request
      and drop every other answer, in the success and in the error handler
- [x] 1.2 Use it in both lists, the available and the configured one
- [x] 1.3 `EntityList.open()` resets the search field and the filter check marks; a reset must not
      start a search of its own
- [x] 1.4 `SearchField.clear()` restores the placeholder and the magnifier icon
- [x] 1.5 `GroupAdd` opens the list through `EntityList.open()` like every other entry point
- [x] 1.6 Ask the core for a forced integration reload only for the load that opens the list
- [x] 1.7 Unit tests: new `testEntitiesStaleResponse` target in `test/ui/CMakeLists.txt`
- [x] 1.8 `CHANGELOG.md` entries under `## Unreleased` / `### Fixed`
- [x] 1.9 Build, `qmllint` on the changed QML files and the full test suite green in CI

## 2. Spec sync (this change)

- [x] 2.1 `entity-management`: MODIFIED "Available and configured entity lists" — one request at a
      time, reset of search and filters on open, forced reload only when the list opens
- [x] 2.2 `entity-management`: MODIFIED "Search and type filter in entity lists" — the list shows the
      result of the newest search text
- [x] 2.3 `design.md` Current State Analysis against `3778fc27` with the merged `file:line`
      references, `adr.md` review manifest (no new ADR)
- [x] 2.4 `openspec validate entity-list-search --strict` green
- [ ] 2.5 Check named as outstanding by the merge commit: run the desktop simulator against the
      Remote-Core Simulator — type fast, reopen the list from every entry point, and confirm with a
      slow integration that only the opening forces a reload

## 3. Archive

- [x] 3.1 Archive this change, which merges the deltas into `openspec/specs/`
