# ADR Review Manifest

## ADR Review Completed

- Date: 2026-09-23
- Reviewer: Markus Zehnder
- Change: voice-start-after-session-end

## In-Force ADR Context Reviewed

All ADRs in `docs/adr/` were read and their `Supersedes:` links walked: 0001–0010 are accepted and
none of them is superseded. Relevant to this change:

- docs/adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md — `voice_start` and `voice_end`
  are unchanged Core-API commands; the UI only withdraws a request of its own that has become
  obsolete, and sends one command fewer. The session id stays the UI's, as it already was.
- docs/adr/0007-one-input-idiom-per-screen.md — the voice overlay keeps its idiom; the extra call in
  its close handler touches no input ownership and no focus.
- docs/adr/0009-unit-tests-for-new-logic-and-bug-fixes.md — the reason the decision was extracted
  into `voiceSession`: it is testable without a core connection, and the reported 503 is covered by
  controller tests.
- docs/adr/0003-static-aarch64-binary-from-the-public-toolchain.md — the two new source files are
  registered in `remote-ui.pro`; the wakeup race itself can only be observed on a device.
- docs/adr/0008-remote-two-and-remote-3-with-feature-parity.md — the microphone button and the
  assistant behave the same on both remotes.
- docs/adr/0002-qt-5-15-lts-pinned.md — Qt 5.15 only.

## Repository-Level ADRs Created

- None. No durable architectural decision was introduced. That a withdrawal of a pending command is
  not gated like the sending of one is a property of this command path, recorded as requirement text
  in `entity-commands`, not an architectural commitment; the wakeup retry policy it builds on is
  likewise spec text (change `wakeup-retry-policy`, commit `ce14d263`).

## Notes

Extracting a pure decision into its own unit so it can be unit-tested is already the standing
commitment of ADR 0009 and needs no ADR of its own.
