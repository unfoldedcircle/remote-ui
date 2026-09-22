# FontAwesome icon font: Free by default, optional Pro at build time

Every icon in the UI that is not a user-supplied image or an emoji is a glyph of an embedded, patched FontAwesome font:
`resources/icons/icon-font.ttf`.  
This page explains why there are two editions, how they differ, and how a firmware build switches to the licensed Pro edition.

The same split exists in the web-configurator, using the same rules.

## The two editions

|          | Free (default: repository, CI, developers, public releases)    | Pro (firmware build only)                                |
| -------- | -------------------------------------------------------------- | -------------------------------------------------------- |
| Source   | `@fortawesome/fontawesome-free`, redistributable (SIL OFL 1.1) | `@fortawesome/fontawesome-pro`, commercially licensed    |
| Style    | Solid, the only full weight the Free edition has               | Light, the design weight of the UI                       |
| Icons    | about 1400 of the mapped icon names resolve                    | about 3300 resolve                                       |
| Runtime  | missing icons are replaced through `icon-fallback.json`        | every mapped icon exists, the fallback never applies     |

- **The Pro font must never be committed.** It is overlaid in a disposable build checkout.
- The tracked font is the Free edition, and CI fails if that changes.

The device binary is built by the `UCR2 aarch64` job of `.github/workflows/build.yml`, and that
job does the overlay itself — see [CI builds](#ci-builds) below.  
Nothing has to be run by hand or commited for a firmware build.

## What is embedded, and why it is patched

`tools/icon-font.py` takes a Font Awesome webfont and patches it in two ways:

- **Emoji code points are unmapped.**  
  Font Awesome maps several hundred emoji code points to icon glyphs, so any text containing an emoji would render as
  icons and conflict with the included Emoji font in the device firmware.
- **The font is renamed** to the family `UC Icons`.  
  Font Awesome Free is SIL OFL 1.1 with the reserved font name "Font Awesome", which a modified version must not carry.
  Renaming both editions to the same family also keeps the QML free of edition-specific names.  
  The copyright, version and license entries of the font are left untouched, as the licence  requires,
  and `resources/icons/LICENSE-fontawesome.txt` ships next to the font.

`resources/icons/icon-font.json` records what the embedded font is:
edition, Font Awesome release, the source file with its checksum, how many emoji code points were unmapped, and the
checksum of the result.

`tools/icon-font.py check` verifies the font against it, which is what CI runs.

## Icon names, fallbacks and the placeholder

`resources/icons/icon-mapping.json` maps an icon name to its code point, and is shared by both
editions, because Font Awesome gives an icon the same code point in every style.

At startup the app reads the family of the font it just loaded and passes it to `Resources`,
which then resolves `uc:<name>` like this:

1. the icon name's code point, when the embedded font can draw it.
2. otherwise the name's entry in `resources/icons/icon-fallback.json`, an icon that exists in the Free edition.
3. otherwise the placeholder from the same file, a question mark in a circle.

The icon selection in the UI lists only icons the embedded font can draw, so a user cannot pick
an icon that would show as the placeholder.

A Pro build resolves everything in step 1, so the fallback file has no effect there.  
Keep it up to date anyway when a new Pro-only icon is used in the UI: run the app once against the Free font and look
for `Icon ... not in the icon font` in the log.

## Rebuilding the font

Both are done with the same script; neither is needed for a normal build.

```shell
pip install fonttools

# Free, e.g. after a Font Awesome upgrade. Commit the result.
tools/icon-font.sh --free
tools/icon-font.sh --free /ci-cache/fontawesome-free

# Pro, in a disposable checkout on the firmware build system. Never commit the result.
FONTAWESOME_NPM_AUTH_TOKEN=<token> tools/icon-font.sh --pro
tools/icon-font.sh --pro /ci-cache/fontawesome-pro
```

With a package directory the script uses it as it is; without one it downloads the package with `npm pack`.  
For Pro that needs the subscription token, either in `FONTAWESOME_NPM_AUTH_TOKEN` or in the `@fortawesome` registry
configuration. The token is written to a temporary `.npmrc` and never persisted.

After a Pro build, restore the tracked font:

```shell
git checkout -- resources/icons/icon-font.ttf resources/icons/icon-font.json
```

The Font Awesome release is pinned in `tools/icon-font.sh` and must stay in step with `icon-mapping.json`:
a different release renumbers icons.

## CI builds

The `UCR2 aarch64` job of `.github/workflows/build.yml` produces the binary that ships in the
firmware, so that is where the Pro font goes in. The job runs `tools/icon-font.sh --pro` over
its own checkout before the cross compile, which is disposable by nature: the overlaid font
exists only for that build and is never pushed anywhere. It ends up inside the static binary
and nowhere else in the artifact.

The switch is the repository secret **`FONTAWESOME_NPM_AUTH_TOKEN`**, the Font Awesome
subscription token: with it the build embeds the Pro font, without it the Free one, and nothing
else in the workflow differs. A repository that has no subscription — the public one, a fork —
therefore builds the Free font from the same workflow file, and needs no change to it.

A build with the Free font is a valid build, so CI does not treat it as an error. Whether the
firmware that ships actually got the licensed font is checked by the firmware build, which is
where the two are brought together.

The job prints what went in, as an `### Icon font` section in the job summary and in the log.
The binary embeds the font but not its provenance file, so that is the record of which edition
an artifact contains.

The unit tests deliberately stay on the Free font: `testIconFont` asserts the fallback
behaviour, which a Pro build does not exhibit.

## Guardrails

- CI runs `tools/icon-font.py check --require-free` over the tracked font. It fails:
  - if the font does not match its provenance file.
  - if the provenance says the Pro edition.
  - if the font still carries the Font Awesome family name.
- The Pro overlay only ever runs in a disposable checkout: a CI runner, or a local checkout where the font is restored in afterwards.
- The overlay verifies the provenance file it just wrote actually says the Pro edition, so a
  download that quietly resolved to the Free package fails the build instead of shipping.
