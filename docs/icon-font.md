# FontAwesome icon font: Free built in, Pro loaded from the firmware

Every icon in the UI that is not a user-supplied image or an emoji is a glyph of a patched FontAwesome font.
The binary embeds the Free edition (`resources/icons/icon-font.ttf`); on the device, the firmware installs the
licensed Pro edition as a file and the app loads that one instead.
This page explains why there are two editions, how they differ, how the app picks one at start-up and how
the firmware build produces the Pro file.

The web-configurator has the same split, with the same rules.

## The two editions

|          | Free (built into every binary)                                 | Pro (file installed by the firmware)                     |
| -------- | -------------------------------------------------------------- | -------------------------------------------------------- |
| Source   | `@fortawesome/fontawesome-free`, redistributable (SIL OFL 1.1) | `@fortawesome/fontawesome-pro`, commercially licensed    |
| Style    | Solid, the only full weight the Free edition has               | Light, the design weight of the UI                       |
| Icons    | about 1400 of the mapped icon names resolve                    | about 3300 resolve                                       |
| Runtime  | missing icons are replaced through `icon-fallback.json`        | every mapped icon exists, the fallback never applies     |
| Where    | `resources/icons/icon-font.ttf`, compiled into `remote-ui`     | a file on the device, named in `UC_ICON_FONT_PATH`       |

- **The Pro font must never be committed**, and it is never built into the binary either. The device build,
  the desktop build and a build from the public sources are one and the same binary.
- The tracked font is the Free edition, and CI fails if that changes.

## How the app picks the font at start-up

1. If `UC_ICON_FONT_PATH` names a readable file and Qt can load it, that font is used.
2. Otherwise the embedded Free font is used, with a warning in the log when a path was set but did not
   work (missing file, unreadable, not a font).

The log line `Icon font loaded: <family> from <path>` says which one is in use. The app reads the family
from the loaded font and passes it to `Resources` and to QML as `fonts.iconFamily`, so nothing in the
UI names an edition.

On the device the firmware sets `UC_ICON_FONT_PATH` in the `remote-ui` service and installs the Pro file
readable by the `remote-ui` user only. A custom UI build installed by a user runs in a separate sandbox
where that file is not available, so it renders with the embedded Free font.

On a desktop, a developer with a Font Awesome Pro subscription can build the Pro file (below) and set
`UC_ICON_FONT_PATH` in the run environment to see the device rendering; without it the simulator shows the
Free rendering, which is also what CI and the unit tests use.

## What is embedded, and why it is patched

`tools/icon-font.py` takes a Font Awesome webfont and patches it in two ways:

- **Emoji code points are unmapped.**  
  Font Awesome maps several hundred emoji code points to icon glyphs, so any text containing an emoji would render as
  icons and conflict with the included Emoji font in the device firmware.
- **The font is renamed** to the family `UC Icons`.  
  Font Awesome Free is SIL OFL 1.1 with the reserved font name "Font Awesome", which a modified version must not carry.
  Renaming both editions to the same family also keeps the QML free of edition-specific names.  
  The copyright, version and license entries of the font are left untouched, as the licence requires,
  and `resources/icons/LICENSE-fontawesome.txt` ships next to the font.

`resources/icons/icon-font.json` records what the embedded font is:
edition, Font Awesome release, the source file with its checksum, how many emoji code points were unmapped, and the
checksum of the result. A Pro file built for the firmware gets the same provenance file next to it.

`tools/icon-font.py check` verifies the tracked font against its provenance, which is what CI runs.

## Icon names, fallbacks and the placeholder

`resources/icons/icon-mapping.json` maps an icon name to its code point, and is shared by both
editions, because Font Awesome gives an icon the same code point in every style.

At startup the app reads the family of the font it just loaded and passes it to `Resources`,
which then resolves `uc:<name>` like this:

1. the icon name's code point, when the loaded font can draw it.
2. otherwise the name's entry in `resources/icons/icon-fallback.json`, an icon that exists in the Free edition.
3. otherwise the placeholder from the same file, a question mark in a circle.

The icon selection in the UI lists only icons the loaded font can draw, so a user cannot pick
an icon that would show as the placeholder.

With the Pro font everything resolves in step 1, so the fallback file has no effect there.  
Keep it up to date anyway when a new Pro-only icon is used in the UI: run the app once with the Free font and look
for `Icon ... not in the icon font` in the log.

## Rebuilding the font

Both editions are produced by the same script; neither is needed for a normal build.

```shell
pip install fonttools

# Free, e.g. after a Font Awesome upgrade. Writes the tracked font. Commit the result.
tools/icon-font.sh --free
tools/icon-font.sh --free /ci-cache/fontawesome-free

# Pro, on the firmware build system. Writes the font and its provenance to the given path,
# never into the repository. Install the file on the device and set UC_ICON_FONT_PATH.
FONTAWESOME_NPM_AUTH_TOKEN=<token> tools/icon-font.sh --pro --output /out/icon-font.ttf
tools/icon-font.sh --pro --output /out/icon-font.ttf /ci-cache/fontawesome-pro
```

With a package directory the script uses it as it is; without one it downloads the package with `npm pack`.  
For Pro that needs the subscription token, either in `FONTAWESOME_NPM_AUTH_TOKEN` or in the `@fortawesome` registry
configuration. The token is written to a temporary `.npmrc` and never persisted. `--pro` refuses to write into
`resources/`, so a Pro build cannot end up in the working tree by accident.

The Font Awesome release is pinned in `tools/icon-font.sh` and must stay in step with `icon-mapping.json`:
a different release adds icons that the mapping does not know, and the script refuses a package of another release.

## CI

The code-guidelines workflow runs `tools/icon-font.py check --require-free` over the tracked font. It fails:

- if the font does not match its provenance file.
- if the provenance says the Pro edition.
- if the font still carries the Font Awesome family name.

The build workflow needs neither a subscription token nor `fonttools`: every job builds the same binary
with the embedded Free font. Whether the firmware that ships actually installed the licensed font is
checked by the firmware build, which is where the two are brought together.

The unit tests run with the Free font: `testIconFont` asserts the fallback behaviour, which a Pro font does
not exhibit, and that an external font is only used when `UC_ICON_FONT_PATH` names a file that loads.
