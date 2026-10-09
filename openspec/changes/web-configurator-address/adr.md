# ADR Review Manifest

## ADR Review Completed

- Date: 2026-10-09
- Reviewer: Markus Zehnder (drafted with Claude Code)
- Change: web-configurator-address

## In-Force ADR Context Reviewed

All ADRs 0001–0018 are accepted and none is superseded; ADRs 0019 and 0020 come with the design-system
change this one is stacked on. Relevant to this change:

- adr/0009-unit-tests-for-new-logic-and-bug-fixes.md - the address rule has a test in testCommon
- adr/0012-qml-is-presentation-only-logic-and-models-in-cpp.md - the three QML bindings that chose the address call one C++ function

## Repository-Level ADRs Created

- None: no major durable architectural decisions were introduced by this change.

## Notes

The change keeps the IP address first and the toggle as they are.
