# ADR Review Manifest

## ADR Review Completed

- Date: 2026-10-09
- Reviewer: Markus Zehnder (drafted with Claude Code)
- Change: license-documents

## In-Force ADR Context Reviewed

All ADRs 0001–0018 are accepted and none is superseded; ADRs 0019 and 0020 come with the design-system
change this one is stacked on. Relevant to this change:

- adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md - the licenses are files of the legal directory; nothing comes from the core or the network
- adr/0009-unit-tests-for-new-logic-and-bug-fixes.md - each fix of the text preparation has a test in testResources
- adr/0012-qml-is-presentation-only-logic-and-models-in-cpp.md - the splitting the page did in QML moves to Resources with the new preparation; the way back stays in the page as navigation
- adr/0019-the-design-system-owns-colours-type-and-selection.md - the prose role, nothing below 22 px, links in textPrimary

## Repository-Level ADRs Created

- None: no major durable architectural decisions were introduced by this change.

## Notes

The change fixes how the Licenses page shows and follows its documents.
