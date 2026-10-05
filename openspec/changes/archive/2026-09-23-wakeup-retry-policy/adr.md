# ADR Review Manifest

## ADR Review Completed

- Date: 2026-09-23
- Reviewer: Markus Zehnder
- Change: wakeup-retry-policy

## In-Force ADR Context Reviewed

All ADRs in `docs/adr/` were read and their `Supersedes:` links walked: 0001–0010 are accepted and
none of them is superseded. Relevant to this change:

- docs/adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md — the failure codes are the
  core's own; the UI only decides whether to send the same request again, it does not interpret the
  device semantics behind a code.
- docs/adr/0008-remote-two-and-remote-3-with-feature-parity.md — the retry window behaves
  identically on both remotes; no model branch was added.
- docs/adr/0009-unit-tests-for-new-logic-and-bug-fixes.md — the reason the decision was extracted
  into `commandRetryPolicy`: it is testable without a core connection, and every code is pinned by a
  test case.
- docs/adr/0003-static-aarch64-binary-from-the-public-toolchain.md — the two new source files are
  registered in `remote-ui.pro`; nothing is loaded from disk.
- docs/adr/0002-qt-5-15-lts-pinned.md — Qt 5.15 only (`QString`, `QVariantMap`, QtTest).

## Repository-Level ADRs Created

- None. No durable architectural decision was introduced. The list of rejected codes (400, 401, 403,
  422, 501) is a behaviour policy, not an architectural commitment: it is recorded as requirement
  text in the `entity-commands` and `power-and-battery` capabilities, where a future change can
  amend it with a normal spec delta.

## Notes

Extracting a pure decision into its own unit so it can be unit-tested is already the standing
commitment of ADR 0009 and needs no ADR of its own.
