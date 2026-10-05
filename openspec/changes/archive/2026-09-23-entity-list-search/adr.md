# ADR Review Manifest

## ADR Review Completed

- Date: 2026-09-23
- Reviewer: Markus Zehnder
- Change: entity-list-search

## In-Force ADR Context Reviewed

All ADRs in `docs/adr/` were read and their `Supersedes:` links walked: 0001–0010 are accepted and
none of them is superseded. Relevant to this change:

- docs/adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md — the lists keep using
  `get_entities` and `get_available_entities`; the guard only decides which answer the model accepts,
  it adds no filtering or business logic of its own, and the expensive `force_reload` is left to the
  core exactly once per opening.
- docs/adr/0007-one-input-idiom-per-screen.md — the list stays on its existing idiom: no
  button-navigation configuration, no focus chain and no zone was changed.
- docs/adr/0009-unit-tests-for-new-logic-and-bug-fixes.md — the stale-response guard is covered by a
  new unit test target through a minimal model subclass; the request path itself needs a core
  connection and is left to the simulator check.
- docs/adr/0008-remote-two-and-remote-3-with-feature-parity.md — the lists are identical on both
  remotes.

## Repository-Level ADRs Created

- None. No durable architectural decision was introduced. Reusing the one-request-at-a-time guard of
  the media browser search, rather than inventing a second mechanism, is a consistency choice
  recorded in `design.md`; the observable behaviour is in the `entity-management` capability.

## Notes

Whether a list resets its search on open is product behaviour, captured as requirement text, not an
architectural commitment.
