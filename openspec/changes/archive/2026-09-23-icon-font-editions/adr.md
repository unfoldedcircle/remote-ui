# ADR Review Manifest

## ADR Review Completed

- Date: 2026-09-23
- Reviewer: Markus Zehnder
- Change: icon-font-editions

## In-Force ADR Context Reviewed

All ADRs under `docs/adr/` were read and the supersession graph built from their `Supersedes`
fields: nothing is superseded, so 0001–0010 are all in force. The ones that constrain this
change:

- `docs/adr/0004-gpl-3-license-and-published-source.md` — GPL-3.0-or-later with published source
  is why only a redistributable font may be tracked, and why third-party assets
  need an approved, compatible licence.
- `docs/adr/0003-static-aarch64-binary-from-the-public-toolchain.md` — the icon font is embedded
  in the one static binary and is not loaded from disk, so the edition is fixed at build time.
- `docs/adr/0002-qt-5-15-lts-pinned.md` — the font is loaded and measured with Qt 5.15 APIs; no
  new Qt module is involved.
- `docs/adr/0010-icon-font-free-embedded-pro-from-the-firmware.md` — the decision this change documents.

## Repository-Level ADRs Created

None. The durable decision — track the Free edition, overlay Pro at build time, rename the
patched font — is already recorded in
`docs/adr/0010-icon-font-free-embedded-pro-from-the-firmware.md` (accepted 2026-09-18), which was written
with the implementation and needs no amendment. ADR 0010 itself notes that `ui-resources` would
need a `MODIFIED` delta once both the icon-font work and the OpenSpec adoption were on `main`;
this change is that delta.

## Notes

No new durable architectural commitment is introduced here: the change is documentation, it
records shipped behaviour in the living `ui-resources` spec, and every decision behind it was
taken in commit `f489354b` under ADR 0010.
