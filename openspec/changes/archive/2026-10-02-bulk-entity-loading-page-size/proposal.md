## Why

Loading all entities after a connection never ended on Apple Silicon builds: the log showed
`page: 5 of 2147483647`, followed by a request for every further page. Every paged request derived
its page count from the `limit` field of the core's answer, but the core reports there the number of
items in *that* page — smaller than the page size on the last page and 0 for a page past the end. The
page count `count / limit` was computed in floating point, so `369 / 0` became infinity, and
converting infinity to `int` is undefined behaviour: x86 yields `INT_MIN`, which ended the loop by
accident, arm64 saturates to `INT_MAX`. Even where the loop did end, the page count was too high
whenever the last page was short, so the UI asked for a page past the end on every load.

## What Changes

- The number of pages of every paged list is the total `count` the core reports divided by the page
  size the UI **requested**, rounded up, and never less than 1. The `limit` field of an answer is no
  longer used for it. The count is computed in integers, so no input can produce an endless loop.
- The bulk load of all configured entities additionally stops on an empty page, so entities removed
  while the load runs cannot extend it and no request is made past the end.
- The same page count is used by all seven paged loads: the bulk entity load, the configured and the
  available entity lists, the integration list, the integration driver list, the integration status
  list and the dock list.
- A unit test pins the page count, including a page size of 0 and a count of 0.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `entity-management`: the bulk load of configured entities and the entity selection lists derive the
  page count from the page size of the request; the bulk load ends on an empty page.
- `integrations`: the integration, driver and status lists derive the page count from the page size
  of the request.
- `docks`: the dock list derives the page count from the page size of the request and ends on an
  empty page.

## Impact

- **Hardware models:** both, Remote Two and Remote 3; the paging is model-independent. The endless
  loop was observed on an Apple Silicon desktop build; the remotes are aarch64 as well (see the open
  question in `design.md`).
- **remote-core dependency:** none new. The change reads the paging `count` of the existing
  `get_entities`, `get_available_entities`, `get_integrations`, `get_integration_drivers`,
  `get_integration_status` and `get_docks` answers and stops relying on their `limit` field.
- **Third-party code:** none added.
- **Code:** `src/util.{h,cpp}` (`Util::pageCount()`), `src/ui/entity/entityController.cpp`,
  `src/ui/entity/configuredEntities.cpp`, `src/ui/entity/availableEntities.cpp`,
  `src/integration/integrationController.cpp`, `src/dock/dockController.cpp`,
  `test/common/test_util.cpp`, `CHANGELOG.md`.
- **Status:** the implementation is **merged** on `main` as commit `8dac75a0`. This change carries
  the behaviour delta only and is archived on creation.
