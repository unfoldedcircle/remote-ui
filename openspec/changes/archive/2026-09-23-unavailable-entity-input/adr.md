# ADR Review Manifest

## ADR Review Completed

- Date: 2026-09-23
- Reviewer: Markus Zehnder
- Change: unavailable-entity-input

## In-Force ADR Context Reviewed

All ten ADRs in `docs/adr/` are accepted and none is superseded, so all ten are in force. Those that
constrain this change:

- docs/adr/0007-one-input-idiom-per-screen.md - the decisive one. The entity control screens are
  `ButtonNavigation` screens with no focus chain, and the fix keeps that: it removes the
  all-or-nothing `ignoreInput` switch and gates the screen's own override config instead, so BACK and
  HOME stay on the base handlers and no key gets two meanings. The light feature pages keep their
  `overrideActive` handlers - which bypass input ownership by design - but scope them to the page on
  screen, which is the mitigation the ADR asks for. No `Keys` handler or `KeyNavigation` chain was
  added to any of these screens.
- docs/adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md - the rule only decides
  whether the UI puts a command on the wire; the entity state it reads is the core's, and a command
  that is sent is still judged by the core.
- docs/adr/0008-remote-two-and-remote-3-with-feature-parity.md - identical on both models.
- docs/adr/0009-unit-tests-for-new-logic-and-bug-fixes.md - honoured: `mayCommandEntity()` is a
  dependency-free unit with unit tests for the four combinations of availability and resume state.
- docs/adr/0003-static-aarch64-binary-from-the-public-toolchain.md - the new `entityCommandPolicy`
  source pair is registered in `remote-ui.pro` and in `test/ui/CMakeLists.txt`; nothing is loaded
  from disk.
- docs/adr/0006-en-us-ts-is-the-only-edited-translation.md - honoured: the one new string
  ("%1 is unavailable") was added to `en_US.ts` only.

Reviewed and not affected: 0001 (workflow), 0002 (Qt 5.15 only), 0004 (no third-party code), 0010
(no icon-font change).

## Repository-Level ADRs Created

- None. The two candidates were weighed and neither clears the bar: *where* the availability check
  is enforced (at the control rather than in the command entry point) and *that* the check is
  suspended while a resume is pending are entity-command behaviour, already recorded in the
  `entity-commands` capability with scenarios, and they are a consequence of the existing decisions -
  ADR 0007 for the input handling and ADR 0005 for keeping the command judgement with the core.
  Neither diverges from an in-force ADR.

## Notes

The removal of `ButtonNavigation.ignoreInput` is worth remembering as the concrete lesson behind ADR
0007: an all-or-nothing input switch on a screen is what made the screen impossible to leave. It is
captured in the `entity-detail-controls` and `key-navigation` capabilities rather than in a new ADR,
because ADR 0007 already states the rule it follows.
