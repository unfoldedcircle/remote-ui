# ADR Review Manifest

## ADR Review Completed

- Date: 2026-10-02
- Reviewer: Markus Zehnder
- Change: review-crashes-and-dropped-requests

## In-Force ADR Context Reviewed

All ADRs in `docs/adr/` were read and their `Supersedes:` links walked: 0001–0016 are accepted and
none of them is superseded. Relevant to this change:

- docs/adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md — **the governing one.** The
  core's request structs decide where `driver_url` and `token` go, even where the Core-API document
  differs; the group's entity list is the core's, so the UI sends the list the user left, an empty
  one included, instead of deciding that an empty list means "no change"; a group event the UI
  cannot apply is dropped because the core's group load is the source of truth. UI and core ship
  together, so no fallback for another field placement is needed.
- docs/adr/0009-unit-tests-for-new-logic-and-bug-fixes.md — the group crash is pinned by
  `testGroupController`, which dies on SIGSEGV without the fix; `testSetupSchema` covers the setting
  without a field; `testEntityController` gained the repeat case, and `testGroupController` calls both forms of
  `updateGroup` through a `QQmlEngine`. Without a reproducing test, verified by reading only: the
  object-lifetime guards (`QPointer`, timer context objects), the field placement of the
  discovered-driver requests, the driver change handler and the dock description key. These are in
  testable C++ (request builders, a controller slot, a parser), so ADR 0009 would ask for tests;
  they are listed as open items in `tasks.md`.
- docs/adr/0012-qml-is-presentation-only-logic-and-models-in-cpp.md — the "left out versus empty"
  distinction of a group's entities lives in `GroupController` (an optional `QVariant`), not in the
  QML dialogs, which keep calling one function.
- docs/adr/0013-logging-through-qt-categories-to-journald.md — every ignored event or value (unknown
  group, invalid repeat mode, setting without a field) is logged through its controller's category;
  none of them carries a secret.
- docs/adr/0008-remote-two-and-remote-3-with-feature-parity.md — every fix is model-independent.

## Repository-Level ADRs Created

- None. The decisions are applications of ADR 0005 and ADR 0009 to individual defects; that a pointer
  into a list another path deletes is a `QPointer`, and that a single-shot timer gets a context
  object, are C++ review items, not architectural commitments.

## Notes

The code review that produced both commits looked for one defect class across all C++ sources; a
checklist for that class (null from `QHash::value()`, lambdas without a context object, variables
read before assignment) would belong in a review guide under `docs/`, not in an ADR.
