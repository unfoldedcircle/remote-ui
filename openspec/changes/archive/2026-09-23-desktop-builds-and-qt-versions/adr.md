# ADR Review Manifest

## ADR Review Completed

- Date: 2026-09-23
- Reviewer: Markus Zehnder
- Change: desktop-builds-and-qt-versions

## In-Force ADR Context Reviewed

All ADRs under `docs/adr/` were read and the supersession graph built from their `Supersedes`
fields: nothing is superseded, so 0001–0010 are all in force. The ones that constrain this
change:

- `docs/adr/0002-qt-5-15-lts-pinned.md` — Qt 5.15 LTS only, the shipped patch level is whatever
  the device toolchain compiles, the desktop development Qt moves before the device. This change
  carries out the desktop half of that, and the ADR is amended with what has been done.
- `docs/adr/0003-static-aarch64-binary-from-the-public-toolchain.md` — one static binary with
  everything embedded, cross-compiled with a public Docker toolchain image, desktop builds stay a
  faithful simulator of the same code. The new x64 images extend that practice to the desktop and
  the ADR is amended to say so.
- `docs/adr/0004-gpl-3-license-and-published-source.md` — the build must be reproducible outside the
  company, which is why the new toolchain images are public and a release ships a runnable
  simulator.
- `docs/adr/0008` (supported models) and `docs/adr/0007` (one input idiom per screen) were
  checked and are untouched: no model and no input path changes.

## Repository-Level ADRs Created

**None.** No new durable architectural commitment was taken. Both decisions that outlive this
change were already recorded and are only brought up to date. ADRs 0002 and 0003 have not been
merged yet — they arrive with the OpenSpec adoption in the same pull request as this change — so
they are amended in place with a dated sentence rather than superseded; the immutability rule
applies from the moment an ADR is merged. Neither the status, the numbering nor what was decided
changes, and `docs/adr/README.md` needs no edit because neither title changes:

- `docs/adr/0002-qt-5-15-lts-pinned.md` — amended 2026-09-23: the desktop development Qt update
  is done (5.15.19 built from source next to the 5.15.2 binaries, selected per shell, the build
  defaulting to the newest installed), continuous integration still builds with 5.15.2, and the
  device toolchain is still 5.15.8. The decision itself — Qt 5.15 LTS only, patch level follows
  the device toolchain, desktop first — is unchanged, so no superseding ADR.
- `docs/adr/0003-static-aarch64-binary-from-the-public-toolchain.md` — amended 2026-09-23: public
  toolchain images now also exist for the x64 desktop, a static Linux build attached to every
  release and an experimental Windows cross-build. The device image is unchanged and the
  decision — one static binary, nothing loaded from disk — is unaffected, so no superseding ADR.

## Notes

The remaining Qt steps stay where ADR 0002 put them: moving continuous integration off 5.15.2 and
moving the device toolchain to 5.15.19 after testing on both remotes. Either would be a new
change, and leaving Qt 5.15 altogether would supersede ADR 0002.
