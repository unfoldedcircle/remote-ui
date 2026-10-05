# ADR Review Manifest

## ADR Review Completed

- Date: 2026-09-16
- Reviewer: Markus Zehnder
- Change: specify-non-functional-requirements

## In-Force ADR Context Reviewed

- docs/adr/0002-qt-5-15-lts-pinned.md - amended: desktop development Qt moves off 5.15.2 first,
  the device toolchain moves to 5.15.19 after testing.
- docs/adr/0003-static-aarch64-binary-from-the-public-toolchain.md - the platform facts and the
  binary size budget in the `platform-constraints` spec; the dependency policy is still open.
- docs/adr/0004-gpl-3-license-and-published-source.md - amended: new third-party code and assets must
  be compatible with Qt under GPL-3.0 and approved by the lead developer; licensed assets such as
  the Font Awesome Pro font are never tracked.
- docs/adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md - amended: UI and core ship
  together, no compatibility with older cores.
- docs/adr/0006-en-us-ts-is-the-only-edited-translation.md - the shipped-languages requirement.
- docs/adr/0007-one-input-idiom-per-screen.md - the input methods and accessibility requirement.

## Repository-Level ADRs Created

- docs/adr/0008-remote-two-and-remote-3-with-feature-parity.md - Remote Two and Remote 3 are
  supported with feature parity except model-specific hardware; YIO is dropped.
- docs/adr/0009-unit-tests-for-new-logic-and-bug-fixes.md - new logic gets unit tests where
  meaningful; a bug fix gets a regression unit test.
- Amended before their first merge: docs/adr/0002-qt-5-15-lts-pinned.md (desktop Qt update first,
  device 5.15.19 after testing), docs/adr/0004-gpl-3-license-and-published-source.md (dependency
  approval, licensed assets) and docs/adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md
  (UI and core ship together; no compatibility with older cores).

## Notes

The maintainer answered most of the questions in two rounds on 2026-09-17. ADRs 0002, 0004 and
0005 were amended in place instead of superseded because none had been merged yet. The change stays open for the
device measurements and the remaining questions in `design.md`.
