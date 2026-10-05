# ADR Review Manifest

## ADR Review Completed

- Date: 2026-09-24
- Reviewer: Markus Zehnder
- Change: entity-screen-registry

## In-Force ADR Context Reviewed

All ADRs under `docs/adr/` were read and the supersession graph built from their `Supersedes`
fields: nothing is superseded, 0001–0016 are in force. The ones that constrain this change:

- `docs/adr/0012-qml-is-presentation-only-logic-and-models-in-cpp.md` — the screen choice is a
  decision, so it moves from six QML string concatenations into one C++ table; QML only asks.
- `docs/adr/0009-unit-tests-for-new-logic-and-bug-fixes.md` — the registry gets its own test
  target, including the resource registration check.
- `docs/adr/0003-static-aarch64-binary-from-the-public-toolchain.md` — every screen is embedded
  in the binary; a registered screen missing from the resources is exactly the failure the test
  now catches before the device does.
- `docs/adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md` — the device class is
  the core's; the UI only maps it to a screen.

## Repository-Level ADRs Created

- None. Where the screen mapping lives is a consequence of ADR 0012; the maintainer's rule that
  an unknown entity type shows no detail screen is behaviour, recorded in the
  `entity-detail-controls` capability.

## Notes

The maintainer's own words: the qrc-path scheme was never designed. This change turns it into a
designed, tested table without changing what any supported entity shows.
