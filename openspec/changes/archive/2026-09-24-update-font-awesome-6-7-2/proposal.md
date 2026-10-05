## Why

The icon set was pinned to Font Awesome 6.5.1 while the web-configurator ships 6.7.2, and the
icon name mapping — the contract between the clients, the stored configurations and the font —
was a hand-maintained file with no record of where its names came from. Updating the release
adds the icons the other client already offers; generating the mapping from the Font Awesome
package makes every future update reproducible and lets CI verify that the sources, the mapping,
the fallbacks and the font agree. **The implementation is on the branch `feat/fontawesome-6.7.2`
(pull request open), stacked on `feat/icon-font-runtime-override`.**

## What Changes

- The icon set is Font Awesome **6.7.2**: the tracked Free font is rebuilt from it, and the
  firmware builds its Pro file from the same release. Fourteen icons are new, none is removed or
  renamed, no code point changed; Font Awesome redrew a few glyphs, which changes what a tile
  using one of them shows.
- The mapping is **generated** (`tools/icon-font.py mapping <package>`): every canonical icon
  name of the release, all editions, plus `resources/icons/icon-mapping-overrides.json` — the
  82 names that are not Font Awesome names (the remote's own pre-Font-Awesome icon names stored
  in existing configurations, and five Font Awesome names the UI deliberately points at another
  icon). Aliases of Font Awesome are not mapped. The mapping has 3891 names.
- CI runs `tools/icon-font.py check-mapping`: overrides applied, every `uc:` name the sources
  use is mapped, every fallback target and the placeholder are drawable by the Free font, every
  Pro-only name the sources use has a fallback.
- `docs/icon-font.md` explains the whole icon path for a new developer.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `ui-resources`: the icon identifiers (mapping size and origin, the overrides) and the icon font
  edition (release and counts).

## Impact

- **Hardware models:** both. Users see the redrawn glyphs on tiles that use them; new icons
  become selectable once the web-configurator offers them.
- **Firmware dependency:** the Pro file is built from 6.7.2; the release pin is shared.
- **Third-party assets:** the same Font Awesome Free (SIL OFL 1.1) at a newer release; the
  overrides file is project data. No new dependency.
- **Code:** `tools/icon-font.py` (two subcommands), `tools/icon-font.sh` (pin), the regenerated
  font, provenance and mapping, the overrides file, the code-guidelines workflow, the docs,
  `CLAUDE.md`, `CHANGELOG.md`. No C++ or QML change.
