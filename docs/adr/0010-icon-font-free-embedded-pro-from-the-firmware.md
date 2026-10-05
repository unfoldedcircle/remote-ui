# ADR 0010 — Icon font: Free edition embedded, licensed edition loaded from a firmware file, one release pin

|                |                                                                                                       |
| -------------- | ----------------------------------------------------------------------------------------------------- |
| **Status**     | Accepted                                                                                              |
| **Supersedes** | — (none)                                                                                              |
| **Date**       | 2026-09-24                                                                                            |
| **Deciders**   | Markus Zehnder                                                                                        |
| **Related**    | [0003](0003-static-aarch64-binary-from-the-public-toolchain.md), [0004](0004-gpl-3-license-and-published-source.md), `docs/icon-font.md`, `openspec/specs/ui-resources` |

## Context

Every icon that is not a user-supplied image is a glyph of one icon font, addressed by a name
(`uc:lightbulb`) through a name-to-code-point mapping. The design weight of the UI is Font Awesome
**Pro Light**, a commercially licensed font. The repository is GPL-3.0-or-later with published
source (ADR 0004), so only a redistributable font may be tracked — and a font compiled into the
executable as a resource is part of the distributed work, which the Pro licence cannot be
reconciled with. A font file the program loads at start-up is a separate work, aggregated with the
binary on the firmware image, exactly how the Poppins and Space Mono text fonts already reach the
UI (ADR 0003 embeds code and QML, and reads firmware-provided data files from disk).

Facts that shape the solution:

- Font Awesome **Free** (SIL OFL 1.1) has the Solid weight only and about 1400 of the mapped icon
  names; Pro Light has about 3400. Both editions give an icon the same code point in every style,
  so one mapping serves both fonts.
- The firmware installs Google's **Noto Color Emoji** font next to the text fonts. Font Awesome maps
  several hundred emoji code points to icon glyphs; left in place, the icon font would win Qt's font
  fallback for those code points and draw a monochrome icon instead of the emoji. Disabling font
  merging instead would take the emoji font out of the running altogether.
- Font Awesome Free's Reserved Font Name is "Font Awesome"; a modified font must not carry it.
- The icon names are the contract between the remote, the web-configurator and the stored
  configurations; the remote had its own icon names before Font Awesome, and those live in users'
  configurations.

The alternative, overlaying the Pro font into the build of the device binary, switched by a
subscription secret, is rejected: it makes the device binary a different artefact from every other
build, puts the licence question inside the binary, needs the secret in this repository's CI, and
needs a "restore the tracked font" step after every local Pro build.

## Decision

- **Every binary embeds the Free edition** (`resources/icons/icon-font.ttf`, Solid weight): the
  device, desktop, CI, unit-test and public builds all carry the same font, and the device binary
  built from the public sources is the one that ships. CI fails if the tracked
  font is not the Free edition or does not match its provenance file.
- **The licensed Pro edition is never committed and never built into a binary.** The firmware
  build produces it with `tools/icon-font.sh --pro --output …`, installs it on the device readable
  by the `remote-ui` user only, and names it in **`UC_ICON_FONT_PATH`** in the service environment.
  At start-up the UI loads that file when it is set, readable and loads as a font; otherwise the
  embedded font, with a warning when a path was set but did not work. Exactly one icon font is
  registered; the log names the family and the path in use.
- **Custom UI builds run in a separate sandbox** on the device where the Pro file is not
  available; they render with the embedded Free font, so the licensed font cannot be retrieved
  through the custom-build API.
- **Every emoji is rendered by the Noto Color Emoji font, never by the icon font.** The icon font
  is only used for `uc:` icons; text, entity names and anything a user or an integration types is
  drawn with the text fonts, and its emoji with Noto Color Emoji.
- **Both editions are patched by the same script** (`tools/icon-font.py`): emoji code points
  unmapped (so that the emoji font wins the fallback), family renamed to `UC Icons`, copyright, version and licence entries kept, licence text
  and a provenance file beside the tracked font. QML never names the font: the app reads the
  family from the loaded font and exposes it as `fonts.iconFamily`.
- **One mapping, generated, canonical names only.** `resources/icons/icon-mapping.json` is
  generated from the Font Awesome package (every canonical icon of the release, all editions,
  aliases excluded) plus `icon-mapping-overrides.json`, the remote's own names and the deliberate
  remaps, each pointing at the Font Awesome icon it shows. A name once offered stays in the
  mapping. `check-mapping` in CI verifies the mapping, the overrides, the fallbacks and the names
  the sources use against each other and the Free font.
- **A mapped name the loaded font cannot draw** is replaced through `icon-fallback.json` by a
  similar Free icon, otherwise by a placeholder; the icon selector offers only drawable names.
- **One release pin** (`FA_VERSION` in `tools/icon-font.sh`) for the embedded font, the mapping and
  the firmware's Pro file; a release bump rebuilds all three together after comparing the
  releases (added, removed, redrawn icons), and the changelog names what changed.
- **The Font Awesome release is shared with the web-configurator** and cannot be updated in the
  Remote-UI alone: both clients show the same icon names, offer them in their icon selectors and
  store them in the same configurations, so a release bump is made in both projects together.

## Consequences

- **Easier:** no licensed material in any input or output of this repository's builds; no secret,
  no overlay, no Python in the build workflow; reproducible artefacts; the GPL question
  is answered by structure rather than by argument; a developer with a subscription can see the
  device rendering on the desktop by setting the variable; the mapping is reproducible and the
  project's own names are documented in one small file.
- **Harder / accepted:** the firmware and the UI ship an icon change together, and a Font Awesome
  update needs the web-configurator to move at the same time; a firmware that
  forgets the file or the variable renders the Free font (Solid weight, placeholders for Pro-only
  icons) — visible on the first screen and logged with the path, but not caught by this
  repository's CI. Builds without the file look different from the device, and the fallback list
  has to be extended whenever the UI starts using a Pro-only icon. Brand icons are in neither
  embedded font and render the placeholder. The family name no longer says "Font Awesome", which
  the licence requires, so the origin is documented in the provenance file instead.
