# ADR Review Manifest

## ADR Review Completed

- Date: 2026-10-09
- Reviewer: Markus Zehnder (drafted with Claude Code)
- Change: translated-error-messages

## In-Force ADR Context Reviewed

All ADRs 0001–0018 are accepted and none is superseded; ADRs 0019 and 0020 come with the design-system
change this one is stacked on. Relevant to this change:

- adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md - the core's messages are shown unchanged; only the UI's own text around them is translated
- adr/0006-en-us-ts-is-the-only-edited-translation.md - en_US.ts is the only catalogue changed; every new text has a translator comment, and none needs a disambiguation

## Repository-Level ADRs Created

- None: no major durable architectural decisions were introduced by this change.

## Notes

The change applies the translation policy of ADR 0006 to texts that bypassed it and fixes the battery check of
the software update.
