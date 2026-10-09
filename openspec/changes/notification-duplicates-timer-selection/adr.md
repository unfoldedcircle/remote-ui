# ADR Review Manifest

## ADR Review Completed

- Date: 2026-10-09
- Reviewer: Markus Zehnder (drafted with Claude Code)
- Change: notification-duplicates-timer-selection

## In-Force ADR Context Reviewed

All ADRs 0001–0018 are accepted and none is superseded; ADRs 0019 and 0020 come with the design-system
change this one is stacked on. Relevant to this change:

- adr/0007-one-input-idiom-per-screen.md - the actionable notification stays a ButtonNavigation layer; its selection is reset on entry, now also on a push
- adr/0009-unit-tests-for-new-logic-and-bug-fixes.md - the de-duplication rule has unit tests in testUiModels
- adr/0012-qml-is-presentation-only-logic-and-models-in-cpp.md - the comparison lives in NotificationItem::isDuplicateOf(); QML only asks

## Repository-Level ADRs Created

- None: no major durable architectural decisions were introduced by this change.

## Notes

The change fixes three defects within the existing notification model and introduces no new pattern,
dependency or boundary.
