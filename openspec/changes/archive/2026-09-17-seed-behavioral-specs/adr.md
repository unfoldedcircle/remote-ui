# ADR Review Manifest

## ADR Review Completed

- Date: 2026-09-16
- Reviewer: Markus Zehnder
- Change: seed-behavioral-specs

## In-Force ADR Context Reviewed

- docs/adr/0001-adopt-openspec.md - the workflow this seeding executes; it asks for the
  standing constraints to be recorded as ADRs 0002–0007 here.

## Repository-Level ADRs Created

- docs/adr/0002-qt-5-15-lts-pinned.md - Qt 5.15 LTS only; the shipped patch level is the device
  toolchain's (5.15.8), upgrades happen through a toolchain rebuild (5.15.19 candidate).
- docs/adr/0003-static-aarch64-binary-from-the-public-toolchain.md - one static binary with
  everything embedded, built with the public Buildroot/Docker toolchain; nothing loaded from disk.
- docs/adr/0004-gpl-3-license-and-published-source.md - GPL-3.0-or-later, published source,
  user-installable custom builds.
- docs/adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md - all state and commands
  over the Core-API WebSocket with the file token; the UI owns no business logic and degrades
  gracefully against older cores.
- docs/adr/0006-en-us-ts-is-the-only-edited-translation.md - `en_US.ts` is the only hand-edited
  translation; other languages are owned by SimpleLocalize.
- docs/adr/0007-one-input-idiom-per-screen.md - two input paths, one idiom per screen, input
  ownership stack with focus management.

## Notes

These ADRs record decisions that were already in force (evident in the code, `CLAUDE.md`, the
toolchain and the release process) and are dated today because that is when they were written
down, not when they were taken. The decision _to seed_ the specs is in the proposal, not an ADR.
The undecided non-functional requirements are deliberately not turned into ADRs here; they are
collected in `specify-non-functional-requirements`.
