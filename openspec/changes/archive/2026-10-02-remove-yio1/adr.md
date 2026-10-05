# ADR Review Manifest

## ADR Review Completed

- Date: 2026-09-24
- Reviewer: Markus Zehnder
- Change: remove-yio1

## In-Force ADR Context Reviewed

All ADRs under `docs/adr/` were read and the supersession graph built from their `Supersedes`
fields: nothing is superseded, 0001–0016 are in force. The ones that constrain this change:

- `docs/adr/0008-remote-two-and-remote-3-with-feature-parity.md` — YIO is dropped; this change
  is the removal that decision calls for.
- `docs/adr/0016-the-ui-touches-as-little-hardware-as-possible.md` — names the `YIO1` removal
  and the re-homing of the Core-API clients under `src/hardware/` as follow-ups; this change is
  them.
- `docs/adr/0003-static-aarch64-binary-from-the-public-toolchain.md` — the deleted layouts were
  never embedded, so no binary changes content.
- `docs/adr/0011-qmake-builds-the-app-cmake-builds-the-tests.md` — moved files are re-registered
  in `remote-ui.pro` and in every test target that compiles them.

## Repository-Level ADRs Created

- None. Both the removal and the re-homing execute decisions already recorded (ADR 0008,
  ADR 0016).

## Notes

`YIO1` was behaviourally identical to `DEV`; removing it changes only how the value is reported
(unknown, with a warning) and removes the name from the documentation.
