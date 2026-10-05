## Context

Retro-documentation of work that is merged. Current State Analysis measured on `main` @
`f489354b`, against the living `ui-resources` spec seeded at `ed901839`:

- **The tracked font changed.** `resources/icons/icons.otf` (3.2 MB, Font Awesome 6 Pro Light,
  no provenance) is gone; `resources/icons/icon-font.ttf` (412 KB) is Font Awesome Free 6.5.1
  Solid, patched. `resources/icons/icon-font.json` records it: `edition: free`,
  `fontAwesomeVersion: 6.5.1`, `family: UC Icons`, 1586 code points, 378 emoji code points
  unmapped, source and result checksums. `LICENSE-fontawesome.txt` ships beside it. The desktop
  binary shrank from 14.5 MB to 11.6 MB.
- **The mapping is unchanged**: `resources/icons/icon-mapping.json` still holds 3877 names and is
  shared by both editions, because an icon has the same code point in every style.
- **Resolution gained two steps.** `src/ui/resources.cpp:65-91` (`getIconGlyph`): a mapped name
  whose glyph the loaded font cannot draw (`canRenderGlyph`, `src/ui/resources.cpp:52-63`, via
  `QFontMetrics::inFontUcs4`) is replaced by its entry in `resources/icons/icon-fallback.json`
  (22 entries plus `placeholder: circle-question`, loaded at `src/ui/resources.cpp:27-37`) and
  otherwise by the placeholder. `src/ui/resources.cpp:202-214` (`getIconList`) filters the icon
  selector list by the same test.
- **A fall-through disappeared.** Before, the `uc:` branch of `getResource` had no `return` and
  an unmapped `uc:` name fell into the TV-channel-icon case
  (`src/ui/resources.cpp@f489354b^:179-198`); now it returns the result of `getIconGlyph`
  (`src/ui/resources.cpp:246-249`). The living spec's "Missing and default icons" still describes
  the fall-through.
- **The family is no longer hard-coded.** `src/ui/uiController.cpp:54-64` loads
  `:icon-font.ttf`, logs `Icon font loaded:` with the registered family and passes it to
  `Fonts::setIconFamily` (`src/ui/fonts.h:16,26-33`) and `Resources::setIconFont`;
  `src/qml/components/Icon.qml:53` and `src/qml/components/VolumeOverlay.qml:176` use
  `fonts.iconFamily`. The living spec's log line ("Icons loaded" / "Icons failed to load") is
  stale.
- **CI.** `.github/workflows/build.yml:198-232`: the `UCR2 aarch64` job runs
  `tools/icon-font.sh --pro` over its own checkout before the cross compile, switched by the
  `FONTAWESOME_NPM_AUTH_TOKEN` secret alone, verifies the provenance it just wrote says `pro`,
  and prints the edition into the job summary. `.github/workflows/code_guidelines.yml:51` runs
  `tools/icon-font.py check --require-free` over the tracked font. `test/ui/test_icon_font.cpp`
  (target `testIconFont`) asserts the fallback, the placeholder and the filtered icon list
  against the Free font.
- **Documentation exists**: `docs/icon-font.md` and ADR 0010, both merged. Only the living specs
  are behind.

## Goals / Non-Goals

**Goals:** state in `ui-resources` what a user sees on a device build versus a public build, and
what happens to a Pro-only icon name in a Free build.

**Non-Goals:** changing any code; extending the fallback list; re-deciding the edition split
(ADR 0010); the Brands font, which neither edition embeds.

## Decisions

Taken in commit `f489354b` and recorded in ADR 0010; repeated here only as far as they are
observable:

- **Free is tracked, Pro is overlaid.** The default everyone builds is the redistributable
  edition; the licensed one exists only inside the firmware binary. The switch is the
  subscription token and nothing else, so a repository or fork without a subscription builds the
  same workflow file unchanged. A Free build is a valid build and is not a CI error.
- **One family name for both editions.** The patched font is renamed, which the Free licence
  requires of a modified font anyway, and the UI reads the family from the loaded font. An
  edition swap is therefore a file swap.
- **Degrade, don't break.** A Pro-only name resolves to a deliberately chosen similar icon, and
  only to the placeholder when there is none — never to an empty box. The icon selector hides
  what it cannot draw, so a user cannot choose an icon that would show as the placeholder.
- **The unit tests stay on the Free font**, because the fallback behaviour is what they assert
  and a Pro build does not exhibit it.

## Risks / Trade-offs

- [The public build looks different: Solid instead of Light, placeholders for Pro-only names] →
  accepted and documented; the fallback list covers the Pro-only names the UI actually uses.
- [A new Pro-only icon is used without a fallback entry] → a public build logs the missing
  fallback and draws the placeholder; `docs/icon-font.md` asks for one run against the Free font
  after a UI change that adds icons.
- [The licensed font is committed again] → the guardrail in the code-guidelines workflow fails
  the build on provenance mismatch, a `pro` provenance or the vendor family name.
- [The Pro overlay silently resolves to the Free package] → the overlay step verifies the
  provenance it wrote says `pro` and fails the build otherwise.
- [Static build surface] → the font is a `.qrc` entry, so a wrong file name fails at runtime
  with an empty icon font; the startup log line naming the loaded family is the check.
- [Emoji regression] → the emoji code points stay unmapped in both editions, measured against a
  reduced font set; a rebuilt font that skipped the patch would draw icons for emoji.

## Migration Plan

Merged in commit `f489354b`. Verified by CI: the code-guidelines workflow checks
the tracked font on every pull request, `testIconFont` covers the fallback, the placeholder and
the filtered icon list, and the device job records the embedded edition in its summary. No device
run and no core are needed for this spec sync; the appearance of the device build is unchanged
because it embeds the same Pro glyphs as before.

## Open Questions

None. The remaining follow-up is operational: keep `icon-fallback.json` in step with the icons
the UI uses.
