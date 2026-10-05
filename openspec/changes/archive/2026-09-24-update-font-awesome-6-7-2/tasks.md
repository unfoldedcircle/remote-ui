The implementation is on the branch `feat/fontawesome-6.7.2` (pull request open), stacked on
`feat/icon-font-runtime-override`. The change stays open until both are merged; it is archived
after `icon-font-runtime-override`, whose icon font edition text this change's delta includes.

## 1. Implementation (on the branch)

- [x] 1.1 `FA_VERSION="6.7.2"` in `tools/icon-font.sh`; tracked Free font and provenance rebuilt
      (1598 code points, 378 emoji code points unmapped)
- [x] 1.2 `tools/icon-font.py mapping <package>`: canonical names from
      `metadata/icon-families.json` or `scss/_variables.scss`, plus
      `resources/icons/icon-mapping-overrides.json`; regenerated mapping with 3891 names
      (14 added, none removed, no code point changed)
- [x] 1.3 `resources/icons/icon-mapping-overrides.json`: the 82 names that are not Font Awesome
      names, each pointing at the Font Awesome icon it shows, with the reason in the file
- [x] 1.4 Regeneration at 6.5.1 from the Pro metadata reproduces the previously tracked mapping;
      metadata and scss give identical results on 6.5.1
- [x] 1.5 `tools/icon-font.py check-mapping` in the code-guidelines workflow
- [x] 1.6 `docs/icon-font.md`: the icon path end to end, the files, the overrides, adding an icon,
      upgrading Font Awesome; `CLAUDE.md` rule; `CHANGELOG.md` entry naming the redrawn glyphs
- [x] 1.7 `check --require-free`, `check-mapping` and `make test` (17/17) pass

## 2. Firmware (outside this repository)

- [ ] 2.1 Build the Pro file from 6.7.2 with `tools/icon-font.sh --pro --output …`

## 3. Spec (this change)

- [x] 3.1 `ui-resources` delta: `MODIFIED` icon identifiers and icon font edition
- [x] 3.2 `openspec validate --all --strict` green
- [x] 3.3 Archived on 2026-09-24 after `icon-font-runtime-override`, both branches merged
