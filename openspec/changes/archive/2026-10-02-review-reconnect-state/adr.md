# ADR Review Manifest

## ADR Review Completed

- Date: 2026-10-02
- Reviewer: Markus Zehnder
- Change: review-reconnect-state

## In-Force ADR Context Reviewed

Walked the `Supersedes:` links of `docs/adr/`: ADRs 0001–0016 are all accepted and none is
superseded, so all sixteen are in force. Relevant to this change:

- docs/adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md — **the governing one.** The
  core owns the power mode, the activity state, the pages, groups, drivers and docks; after a
  connect the UI brings its copy to what the core answers and does not derive changes of its own (an
  equal power mode, the Unavailable it set itself on the disconnect, a superseded answer). UI and
  core ship in one firmware release, so no fallback for another core is needed.
- docs/adr/0009-unit-tests-for-new-logic-and-bug-fixes.md — met for the activity case by
  `testEntityController::activity_onAgainAfterReconnect_isNotStartedExternally`, which fails without
  the fix. Not met for the power mode transition, the page and group guards, the driver load and the
  dock reconciliation, which have no unit test; recorded as an open question in `design.md`.
- docs/adr/0012-qml-is-presentation-only-logic-and-models-in-cpp.md — the fix is in C++ (`Power`,
  `Battery`, `Activity`, the controllers). The charging-screen decision itself stays in QML
  (`main.qml`), an existing deviation this change did not move; recorded as an open question.
- docs/adr/0003-static-aarch64-binary-from-the-public-toolchain.md and
  docs/adr/0016-the-ui-touches-as-little-hardware-as-possible.md — the power and battery behaviour
  needs a device to verify; no file was added, so no registration.
- docs/adr/0015-start-up-shows-feedback-first-then-connects.md — the first connect after the start
  runs the same reload code, so the start-up on the charger is part of the behaviour recorded here.
- docs/adr/0013-logging-through-qt-categories-to-journald.md — the new "ignoring stale answer" lines
  log through the existing categories and carry no secret.
- docs/adr/0008-remote-two-and-remote-3-with-feature-parity.md — no model branch was added.
- docs/adr/0001, 0002, 0004, 0006, 0007, 0010, 0011, 0014 — reviewed, not affected.

## Repository-Level ADRs Created

- None. Every decision is a defect correction against ADR 0005, recorded as requirement text in the
  modified capabilities. "The newest request wins" and "a load others wait for always settles" are
  local patterns; if a later change wants them as project-wide rules for every Core-API load, it can
  propose an ADR then.
