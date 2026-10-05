# ADR Review Manifest

## ADR Review Completed

- Date: 2026-09-24
- Reviewer: Markus Zehnder
- Change: update-font-awesome-6-7-2

## In-Force ADR Context Reviewed

All ADRs under `docs/adr/` were read and the supersession graph built from their `Supersedes`
fields: nothing is superseded, 0001–0012 are in force. The ones that constrain this
change:

- `docs/adr/0010-icon-font-free-embedded-pro-from-the-firmware.md` — the Free
  edition is embedded, the Pro edition is a firmware file, and the release is pinned once for both
  editions and the mapping; this change bumps that pin and keeps the three in step.
- `docs/adr/0004-gpl-3-license-and-published-source.md` — only redistributable assets are
  tracked: the regenerated font is Font Awesome Free (SIL OFL 1.1), the overrides file is project
  data, and the licensed package is read only on the maintainer's machine and by the firmware.
- `docs/adr/0011-qmake-builds-the-app-cmake-builds-the-tests.md` — tooling is not qmake: the
  generator and the check are Python subcommands of the existing tool.
- `docs/adr/0009-unit-tests-for-new-logic-and-bug-fixes.md` — no C++ logic changed;
  `testIconFont` runs against the new font, and the new Python check is exercised by CI on every
  change.

## Repository-Level ADRs Created

- None. Generating the mapping from the Font Awesome package with a small overrides file is
  tooling under the decision ADR 0010 already records (one release pin for font, mapping and
  firmware file); it is described in `docs/icon-font.md` and in the `ui-resources` capability, not
  in a new ADR.

## Notes

The overrides file is the first written record of which icon names are the remote's own; the
`ui-resources` delta names it so that the contract with stored configurations is in the spec.
