# ADR Review Manifest

## ADR Review Completed

- Date: 2026-09-23
- Reviewer: Markus Zehnder
- Change: profile-handling

## In-Force ADR Context Reviewed

Walked the `Supersedes:` links of `docs/adr/`: ADRs 0001–0010 are all accepted and none is
superseded, so all ten are in force. Relevant to this change:

- docs/adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md — **the governing one.** The
  core owns the profiles: the restriction comes from the profile object of the `profile_change`
  event and is never derived locally, switching is only persisted through `switch_profile` (the
  removed local-only switch violated this), and the factory reset token is the core's.
  UI and core ship in one firmware release, so no fallback for a core that puts `restricted`
  elsewhere is needed.
- docs/adr/0008-remote-two-and-remote-3-with-feature-parity.md — profile handling is identical on
  both remotes; no model-specific behaviour is introduced.
- docs/adr/0009-unit-tests-for-new-logic-and-bug-fixes.md — satisfied: the event parser was made
  public and a regression test target was added that fails without the fix.
- docs/adr/0003-static-aarch64-binary-from-the-public-toolchain.md — the removed QML screen was
  also removed from the resource file, so it is no longer compiled into the binary.
- docs/adr/0007-one-input-idiom-per-screen.md — the no-profile path keeps its existing idiom; the
  removed screen had none (it could not be left with BACK or HOME).
- docs/adr/0006-en-us-ts-is-the-only-edited-translation.md — honoured: only `en_US.ts` was edited
  for the one new message.
- docs/adr/0001, 0002, 0004, 0010 — reviewed, not affected by this change.

## Repository-Level ADRs Created

- None. No durable architectural decision appeared: every decision here is the correction of a
  defect against commitments ADR 0005 already makes, or a local UI choice (removing an unreachable
  screen, opening a confirmation from a response) recorded in the `profiles` and `app-startup`
  capabilities.

## Notes

The rule that a destructive confirmation is only offered once it can actually be carried out is
stated as a requirement in `profiles`, not as an ADR: it is one screen's behaviour, not a project
-wide commitment. If it turns out to apply to other destructive flows (power off, delete entity,
dock factory reset), a later change can promote it.
