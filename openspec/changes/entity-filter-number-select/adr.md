# ADR Review Manifest

## ADR Review Completed

- Date: 2026-11-14
- Reviewer: Markus Zehnder (drafted with Claude Code)
- Change: entity-filter-number-select

## In-Force ADR Context Reviewed

ADRs 0001–0021 were reviewed; ADR 0019 is superseded by ADR 0021, all others are in force. These
are relevant to this change:

- adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md - The core filters the entities;
  the UI only sends the ticked types in `entity_types`.
- adr/0006-en-us-ts-is-the-only-edited-translation.md - Only `en_US.ts` gets the two new strings.
- adr/0008-remote-two-and-remote-3-with-feature-parity.md - The sheet is the same on both models.
- adr/0009-unit-tests-for-new-logic-and-bug-fixes.md - No new logic: the change is QML list data
  and the existing C++ type mapping is unchanged.
- adr/0010-icon-font-free-embedded-pro-from-the-firmware.md - Both icons have a Free fallback.
- adr/0018-the-ui-tolerates-core-api-values-it-does-not-know.md - Unchanged; `number` and
  `select` are known types.

## Repository-Level ADRs Created

None. No durable architectural decision is introduced.
