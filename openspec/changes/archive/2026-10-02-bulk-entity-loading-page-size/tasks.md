The implementation is merged on `main` as commit `8dac75a0`. This change carries the behaviour
delta, which the archive merges into the living specs.

## 1. Implementation (merged in commit `8dac75a0`)

- [x] 1.1 `src/util.{h,cpp}`: `Util::pageCount(itemCount, pageSize)`, integer page count of at least
      1, documented to take the page size of the request, never the `limit` of a response
- [x] 1.2 `EntityController::loadAllEntities()`: request with `pageSize = 100`, page count from it,
      and stop on an empty page as well as on the last page
- [x] 1.3 `ConfiguredEntities` and `AvailableEntities`: page count from the request's `limit`
- [x] 1.4 `IntegrationController`: integration status, driver and integration lists use the
      request's `limit`
- [x] 1.5 `DockController`: dock list uses the request's `limit`
- [x] 1.6 Rename the shadowing lambda parameter to `responseLimit` and mark it unused in all seven
      handlers
- [x] 1.7 Unit test `testCommon::pageCount` in `test/common/test_util.cpp` (no CMake change needed,
      `util.cpp` is already compiled by the target); it fails for the page size 0 without the fix
- [x] 1.8 No new file, so no `remote-ui.pro`, `.qrc` or CMakeLists registration
- [x] 1.9 `CHANGELOG.md` entry under `## Unreleased` / `### Fixed`
- [x] 1.10 Linux build, `cpplint.sh` and all unit tests green; bulk load verified on an Apple Silicon
      Mac with the static macOS build (ends at page 4 of 4)

## 2. Spec sync (this change)

- [x] 2.1 `entity-management`: MODIFIED "Bulk load of configured entities" — page count from the
      request's page size, at least 1, an empty page ends the load
- [x] 2.2 `entity-management`: MODIFIED "Available and configured entity lists" — page count from the
      request's page size
- [x] 2.3 `integrations`: MODIFIED "Integration and driver lists are loaded from the core" — page
      count from the request's page size, an empty driver page ends the driver load
- [x] 2.4 `docks`: MODIFIED "Configured docks are loaded from the core" — page count from the
      request's page size, an empty page ends the load
- [x] 2.5 `design.md` Current State Analysis with the merged `file:line` references, `adr.md` review
      manifest (no new ADR)
- [x] 2.6 `openspec validate bulk-entity-loading-page-size --strict` green
- [x] 2.7 Archive this change, which merges the deltas into `openspec/specs/`

## 3. Device checks (owed)

- [ ] 3.1 Remote Two: with more than 100 configured entities and a short last page, the bulk load
      ends at the last page (`page: N of N` in the journal) and requests no page past the end
- [x] 3.2 Remote 3: the same check as 3.1 (497 entities in pages 1 to 5 of 5, at 5 starts and 10
      reloads; overnight run of 2026-10-04, `docs/measurement-results.md`)
- [ ] 3.3 Remote Two and Remote 3: after a reconnect the integration driver, integration and dock
      lists are complete
