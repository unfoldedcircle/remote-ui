# ADR Review Manifest

## ADR Review Completed

- Date: 2026-11-03
- Reviewer: Markus Zehnder (drafted with Claude Code)
- Change: no-value-placeholder

## In-Force ADR Context Reviewed

All ADRs 0001–0020 are accepted and none is superseded. Relevant to this change:

- adr/0006-en-us-ts-is-the-only-edited-translation.md - "—" is a symbol and not translated; only `en_US.ts` changes
- adr/0008-remote-two-and-remote-3-with-feature-parity.md - no hardware involved; both models run the same code
- adr/0009-unit-tests-for-new-logic-and-bug-fixes.md - the climate state line test expects the new placeholder
- adr/0019-the-design-system-owns-colours-type-and-selection.md - the placeholder is a design system rule

## Repository-Level ADRs Created

- None: no major durable architectural decisions were introduced by this change.

## Notes

The change replaces a text and introduces no new pattern, dependency or boundary.
