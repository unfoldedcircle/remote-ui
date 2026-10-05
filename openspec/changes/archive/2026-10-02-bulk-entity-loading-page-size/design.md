## Context

Current State Analysis, measured on `feat/openspec` at `3768902e`, which contains the merge commit
`8dac75a0` (`main`, 2026-09-29) and the later reconnect fix `8e97731e`.

- **Before the change**, all seven paged response handlers took the page size from the answer:
  the lambda parameter `limit` of `onResponseWithErrorResult` shadowed the request's `limit` and was
  the `limit` field of the response, which the core fills with the number of items in that page. The
  page count was `qCeil(static_cast<float>(count) / static_cast<float>(limit))`. With 369 entities and
  pages of 100, page 4 carries 69 items, so the count became `ceil(369 / 69) = 6`; page 5 is past the
  end and carries 0 items, so the count became `369 / 0 = inf`, and `int(inf)` is undefined: `INT_MIN`
  on x86 (the loop `pageNum < totalPages` ended), `INT_MAX` on arm64 (it never ended).
- **The page count now** is `Util::pageCount(itemCount, pageSize)` (`src/util.cpp:31`, documented in
  `src/util.h:28`–`36`): `(itemCount + pageSize - 1) / pageSize` in integers, and 1 for a count or a
  page size of 0 or below. No floating point is involved.
- **Bulk entity load:** `EntityController::loadAllEntities()` requests `pageSize = 100`
  (`src/ui/entity/entityController.cpp:474`–`475`), names the response field `responseLimit` and
  marks it unused (`:479`–`480`), computes `Util::pageCount(count, pageSize)` for every page (`:486`)
  and requests the next page only `if (pageNum < totalPages && !entities.isEmpty())` (`:499`); the
  else branch removes the entities not seen and emits `allEntitiesLoaded` as before.
- **Entity selection lists:** `ConfiguredEntities::loadFromCore()` and `AvailableEntities::loadFromCore()`
  compute `Util::pageCount(count, limit)` with the request's `limit` on the first page only
  (`src/ui/entity/configuredEntities.cpp:94`–`95`, `:109`; `src/ui/entity/availableEntities.cpp:97`–`98`,
  `:112`); the page size is `DEFAULT_LIMIT = 100` (`src/ui/entity/availableEntities.h:35`), and
  `Entities::canLoadMore()` is `m_totalPages != m_lastPageLoaded` (`src/ui/entity/entities.cpp:244`).
- **Integrations:** status, drivers and integrations use `Util::pageCount(count, limit)` with the
  request's `limit` (`src/integration/integrationController.cpp:112`–`113`/`:123`,
  `:173`–`174`/`:186`, `:221`–`222`/`:230`); the follow-up pages are requested from
  `onIntegrationStatusLoaded()` (`:1068`–`1071`), `onIntegrationDriverSettled()` (`:1087`–`1090`) and
  `onIntegrationsLoaded()` (`:1105`–`1107`). The driver load ends on an empty page (`:191`); that
  check came with the later commit `8e97731e`, not with `8dac75a0`.
- **Docks:** `DockController::loadDocks()` uses `Util::pageCount(count, limit)` (`src/dock/dockController.cpp:300`–`301`,
  `:310`) and continues `if (page < m_configuredDocks.totalPages && !docks.isEmpty())` (`:333`); the
  empty-page check there also came with `8e97731e`.
- **Test:** `testCommon::pageCount()` (`test/common/test_util.cpp:45`–`56`) covers 369/100 → 4,
  300/100 → 3, 301/100 → 4, 1 and 100 items → 1, and 0 items, page size 0 and negative inputs → 1.

## Goals / Non-Goals

**Goals:** a page count that is right and finite on every platform; never ask for a page past the
end in the bulk load; one helper for every paged list, with a unit test.

**Non-Goals:** changing the page size (100), the order of the start-up loads, the stale-answer
handling of the lists, or the media browser's paging (its own QML code with 20 items per page,
untouched). No cursor-based paging.

## Decisions

- **D1 — The page size is the one the UI requested.** The response's `limit` is the number of items in
  that page, so it cannot serve as the page size on the last page or past the end. _Alternative
  rejected:_ clamping the response's `limit` to at least 1 — the count would still be wrong for a short
  last page and the UI would keep asking for a page past the end.
- **D2 — Integer arithmetic, at least 1.** `(count + size - 1) / size` cannot overflow into an
  undefined conversion; a count of 0 still means one page, which is the page already received. An
  unusable page size (0 or below) yields 1 instead of a division by zero.
- **D3 — The bulk load also stops on an empty page.** The count of the latest answer bounds the loop,
  but an entity removed while the load runs can make a later page empty; stopping there ends the load
  as after the last page. _Alternative rejected:_ relying on the count alone — correct only while
  nothing changes during the load.
- **D4 — The shadowing lambda parameter is renamed `responseLimit` and marked unused**, so the
  request's `limit` captured by the lambda is the one in scope; the comment states why.
- **D5 — The helper lives in `Util`**, which every handler already reaches and which `testCommon`
  already compiles, so no CMake source list had to change.

## Risks / Trade-offs

Failure Mode Analysis — the change sits on the Core-API response handling of the start-up and
reconnect loads:

- **[An empty page ends the bulk load early while entities are still to come]** → a page is only empty
  past the end of the list; an entity added meanwhile arrives as a `NEW` `entity_change` event.
- **[Entities removed during the bulk load shift later entities to an earlier page, so one is never
  seen and is removed locally]** → pre-existing to offset paging and not introduced here; the core
  reports the deletion by event, and the next reconnect reloads everything.
- **[A list's page count is fixed by its first answer while the count changes]** → unchanged from
  before; the entity lists reload from page 1 on every search, filter and opening.
- **[A core that one day reports `limit` as the page size]** → irrelevant now, the field is not read.

Resource impact: none, or a saving. The change replaces a float division by an integer one per
answer and removes the request for a page past the end that every load with a short last page used to
make; on arm64 it removes an endless stream of requests. CPU, memory, binary size and latency are
otherwise untouched.

## Migration Plan

No migration: nothing is stored and the page size is unchanged. Merged in commit `8dac75a0`; verified
by its unit test (`testCommon::pageCount`), by CI (Linux build, `cpplint`, all unit tests) and on an
Apple Silicon Mac with the static macOS build, where the bulk load ended at page 4 of 4. Most of the
behaviour can be checked against the Remote-Core Simulator; whether the remotes themselves were
affected needs a device with the real core (docs/adr/0003).

## Open Questions

- **Device check owed:** on a Remote Two and a Remote 3 with more than 100 configured entities and a
  short last page, the bulk load ends at the last page (`page: N of N` in the journal) and no page past
  the end is requested. The devices are aarch64 like Apple Silicon, so the old code should have looped
  there too whenever the last page was short; the merge commit names no device observation, so it is
  open whether the device core fills `limit` the same way as the simulator.
- **Device check owed:** the integration driver, integration and dock lists load completely after a
  reconnect on the device.
- No in-force ADR is put in question by this change.
