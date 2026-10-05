# ADR Review Manifest

## ADR Review Completed

- Date: 2026-09-24
- Reviewer: Markus Zehnder
- Change: icon-font-runtime-override

## In-Force ADR Context Reviewed

All ADRs under `docs/adr/` were read and the supersession graph built from their `Supersedes`
fields: nothing is superseded. The ones that constrain this change:

- `docs/adr/0004-gpl-3-license-and-published-source.md` — the source of every release is
  published and only redistributable assets are tracked; the licensed font must stay out of the
  repository and, with this change, out of every build of it.
- `docs/adr/0003-static-aarch64-binary-from-the-public-toolchain.md` — one static binary that
  loads no code or QML from disk; firmware-provided data files (token, custom icons, legal
  texts, sound effects, the text fonts) are read from disk. The icon font file joins that list
  as data, so the decision is honoured, not diverged from.
- `docs/adr/0012-qml-is-presentation-only-logic-and-models-in-cpp.md` and
  `docs/adr/0009-unit-tests-for-new-logic-and-bug-fixes.md` — the selection logic is a C++ unit
  with tests.

## Repository-Level ADRs Created

- docs/adr/0010-icon-font-free-embedded-pro-from-the-firmware.md - rewritten in place, because
  no release shipped with its earlier build-time overlay: every binary embeds the Free edition;
  the licensed edition is a firmware-installed file named in `UC_ICON_FONT_PATH`, loaded at
  start-up with the embedded font as fallback; no build-time overlay, no subscription secret in
  this repository; one release pin for font, mapping and firmware file.

## Notes

ADR 0010 was first written for the build-time overlay; since nothing was released with it, it
is rewritten rather than superseded, so the record starts clean. The overlay is mentioned in its
context as the alternative that was tried.
