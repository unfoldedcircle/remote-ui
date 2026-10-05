# ADR Review Manifest

## ADR Review Completed

- Date: 2026-09-23
- Reviewer: Markus Zehnder
- Change: startup-and-shutdown-logging

## In-Force ADR Context Reviewed

Walked the `Supersedes:` links of `docs/adr/`: ADRs 0001–0010 are all accepted and none is
superseded, so all ten are in force. Relevant to this change:

- docs/adr/0003-static-aarch64-binary-from-the-public-toolchain.md — the device runs one static
  binary under systemd and logs to the journal, which is why a shutdown flood is a real cost and
  not cosmetic; sound effects are among the few files read from the file system, so their absence
  must be an ordinary, quiet state.
- docs/adr/0002-qt-5-15-lts-pinned.md — the teardown order is Qt 5.15 behaviour; no Qt 6 API is
  used for it.
- docs/adr/0008-remote-two-and-remote-3-with-feature-parity.md — nothing here is model-specific.
- docs/adr/0007-one-input-idiom-per-screen.md — the input filter is part of that contract; holding
  the filtered window weakly does not change which path a key takes.
- docs/adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md — untouched: no Core-API
  message is involved.
- docs/adr/0009-unit-tests-for-new-logic-and-bug-fixes.md — reviewed and not satisfied by a test
  here: process teardown order and log output have no meaningful unit test in this project; the
  pull request verified them by repeated instrumented runs instead (12 stop runs, 5
  AddressSanitizer runs), which is recorded in design.md.
- docs/adr/0001, 0004, 0006, 0010 — reviewed, not affected by this change.

## Repository-Level ADRs Created

- None. No durable architectural decision was introduced. Destroying the QML engine before the
  objects it refers to is a correction of a lifetime bug, not a new architectural commitment, and
  the logging rules it obeys are already project convention (`src/logging.h` categories) rather
  than a decision this change takes.

## Notes

Whether a missing optional resource should be an info line and a missing *configured* one a
warning is stated in the `hardware-platform` requirement for sound effects. It is a candidate for a
project-wide logging convention if the same question comes up for custom icons, legal texts or
background images; it is not promoted to an ADR on one instance.
