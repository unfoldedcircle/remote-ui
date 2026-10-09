# ADR Review Manifest

## ADR Review Completed

- Date: 2026-10-09
- Reviewer: Markus Zehnder (drafted with Claude Code)
- Change: switch-toggle-fallback

## In-Force ADR Context Reviewed

All ADRs 0001–0018 are accepted and none is superseded. Relevant to this change:

- adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md - the UI picks `on` or `off` from the state the core reported, as the Core-API defines for a switch without `toggle`
- adr/0008-remote-two-and-remote-3-with-feature-parity.md - no hardware involved; both models run the same code
- adr/0009-unit-tests-for-new-logic-and-bug-fixes.md - the fix comes with a unit test that fails without it
- adr/0012-qml-is-presentation-only-logic-and-models-in-cpp.md - the choice lives in `Switch::toggle()`, not in the QML call sites

## Repository-Level ADRs Created

- None: no major durable architectural decisions were introduced by this change.

## Notes

The change applies the rule `Light::toggle()` already follows to the switch entity and introduces
no new pattern, dependency or boundary.
