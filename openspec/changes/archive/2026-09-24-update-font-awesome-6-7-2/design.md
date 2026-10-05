## Context

Current State Analysis, measured on the branch `feat/icon-font-runtime-override` (the base) and
on the Font Awesome packages 6.5.1 and 6.7.2 (Free from npm, Pro from the maintainer's copy):

- **The mapping was hand-maintained.** `resources/icons/icon-mapping.json` (3877 names) was
  last regenerated from a Font Awesome 6.6-era source and then patched by hand (`fix: old icon
  mappings`, 2024-12); nothing recorded which names are Font Awesome's and which are the
  remote's own. Regenerating it from the 6.5.1 Pro metadata plus an override list reproduces it
  exactly (the 38 names newer than 6.5.1 are the only difference), which proves the structure:
  3762 canonical Font Awesome names + 82 project names, 5 of which shadow a Font Awesome name.
- **The remote's own names predate Font Awesome.** `activity`, `apps`, `bw`, `ff`, `lamp-1`,
  `wifi-01`, `profile-hat`, … were the icon names of the first UI; the web-configurator carries
  the same list as its "old to new icon mapping". They are stored in users' configurations and
  must resolve forever.
- **6.5.1 → 6.7.2, measured on the fonts:** identical metrics (512 upm, 460/-75), no hinting
  tables in either release, TrueType outlines; 0 code points removed, 42 added to Light; 14
  canonical names new to the mapping (`carpool`, `chart-diagram`, `chart-fft`, `chart-sine`,
  `circles-overlap-3`, `comment-nodes`, `css`, `file-fragment`, `file-half-dashed`,
  `files-pinwheel`, `hexagon-nodes`, `hexagon-nodes-bolt`, `square-binary`, `square-bluesky`).
  Rasterised at 24, 40 and 80 px, ~50 Light glyphs and 17 Solid glyphs differ visibly (more than
  5 % of their ink); none is used by the UI's own screens (83 literal `uc:` names in `src/`).
- **The web-configurator's icon list** (3275 names) is a subset of the mapping; the two clients
  agree on names.
- **Font Awesome's package formats:** `metadata/icon-families.json` lists every icon with its
  canonical name, unicode, aliases and editions; `scss/_variables.scss` lists `$fa-var-<name>`
  per name with the canonical name first per code point and aliases after it. On 6.5.1 both give
  the same 3762 canonical names.

## Goals / Non-Goals

**Goals:** the same icon release as the web-configurator; a reproducible mapping with the
project's own names in one small, documented file; a CI check that the sources, the mapping,
the fallbacks and the font agree; documentation a new developer can follow.

**Non-Goals:** mapping Font Awesome aliases (one name per icon, as the web-configurator does);
changing any override or fallback; the Brands font; the icon selector's list, which follows the
mapping; any C++ or QML change.

## Decisions

- **D1 — Canonical names only.** The mapping lists each icon once, under Font Awesome's
  canonical name. Aliases (`home`, `chain`) are not mapped: the icon selector and the
  web-configurator list one name per icon, and a stored name must stay unambiguous.
- **D2 — Overrides name the target icon, not a code point.** `icon-mapping-overrides.json` maps
  `lamp-1` to `lamp-floor`, so a release upgrade that moves nothing keeps every override valid
  without editing it, and the generator refuses an override whose target no longer exists.
- **D3 — The generator accepts the metadata or the scss variables.** The metadata is the
  reference; a trimmed package copy (webfonts and scss only) still works, because the scss lists
  the canonical name first per code point — verified against the 6.5.1 metadata with no
  difference.
- **D4 — `check-mapping` in CI, without the licensed package.** CI has neither the Pro package
  nor a token, so it cannot regenerate; it verifies what it can: overrides applied (an override
  whose target is itself overridden is skipped and left to the generator), every `uc:` literal
  in `src/` mapped, fallbacks and placeholder drawable by the tracked Free font, every Pro-only
  name the sources use covered by a fallback.
- **D5 — Redrawn glyphs are accepted and named in the changelog.** A glyph redesign is Font
  Awesome's call and reaches every client; the release note lists the built-in ones so that a
  support question about a changed tile has an answer.

## Risks / Trade-offs

- [A future release removes or renames an icon] → the generator drops the name; stored
  configurations would show nothing. Mitigation: compare the releases before bumping (the
  procedure is in `docs/icon-font.md`) and keep the old name alive as an override. Font Awesome
  has not removed a name in 6.x.
- [The scss heuristic breaks in a future release] → the metadata is the reference; a package
  with metadata is preferred, and the two can be cross-checked as done here.
- [`check-mapping` only sees literal `uc:` names] → names built at runtime (`"uc:" + type`) are
  not checked; the runtime log (`Cannot find icon`) remains the net for those.
- [The 6.5.1-vs-6.7.2 render comparison used the maintainer's Pro packages] → the tooling for
  it was throwaway; the procedure is described in the docs, not shipped.

## Migration Plan

1. Merge `feat/icon-font-runtime-override` (base), then `feat/fontawesome-6.7.2`.
2. Firmware: build the Pro file from 6.7.2 (`tools/icon-font.sh --pro --output …`).
3. Archive `icon-font-runtime-override` first, then this change: both modify the icon font
   edition requirement and this change's text includes the other's.

Rollback is reverting the commit; the previous font and mapping return unchanged.

## Open Questions

None.
