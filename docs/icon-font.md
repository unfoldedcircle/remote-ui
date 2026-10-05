# Icons: the FontAwesome icon font, the name mapping and the two editions

Every icon in the UI that is not a user-supplied image or an emoji is a glyph of a patched FontAwesome font, addressed
by a name such as `uc:lightbulb`. This page is the whole story for a developer who touches icons: how a name becomes a
glyph, which files take part, where the two editions of the font come from, how to add an icon, and how to upgrade
Font Awesome. The web-configurator has the same split, with the same rules.

## How an icon gets on the screen

1. **The identifier.** Every icon is a string `<prefix>:<name>` (`src/ui/resources.cpp`, `Resources::getResource`).
   `uc:` (any prefix containing `uc`) is a glyph of the icon font; `custom:` is an image file from the firmware's
   resource directory (`UC_RESOURCE_PATH/Icon/`); `ctv:` a TV channel icon file. Identifiers come from three places:
   the QML and C++ sources (a hard-coded `uc:` name, e.g. the default icon of an entity type), the core (the icon a
   user chose for an entity, page or activity in the web-configurator, stored in the configuration and sent to the
   remote), and the icon selector in the remote's own settings.
2. **The mapping.** `resources/icons/icon-mapping.json` maps a `uc:` name to the code point of its glyph:
   `"lightbulb": ""`. It is embedded in the binary and loaded by `Resources` at start-up. A name that is not in
   the mapping renders nothing and is logged (`Cannot find icon:`).
3. **The font.** Exactly one icon font is registered at start-up (`src/ui/iconFont.cpp`): the file named in
   `UC_ICON_FONT_PATH` when the firmware provides one, otherwise the embedded `resources/icons/icon-font.ttf`. Both
   carry the family name `UC Icons`; the app reads the family from the loaded font and hands it to QML as
   `fonts.iconFamily` and to `Resources`, so nothing in the UI names a font or an edition.
4. **Can the font draw it?** `Resources::getIconGlyph` asks the loaded font (`QFontMetrics::inFontUcs4`) whether it has
   the glyph. If yes, the glyph is returned and `components/Icon.qml` draws it as text in `fonts.iconFamily` at half
   the box size. If not — the name exists only in the Pro edition and the loaded font is Free — the name's entry in
   `resources/icons/icon-fallback.json` is drawn instead (`keyboard-down` → `keyboard`), and without an entry the
   placeholder from the same file, a question mark in a circle. Both substitutions are logged
   (`Icon ... not in the icon font`).
5. **The icon selector** (Settings) offers only the names the loaded font can draw (`Resources::getIconList`), so a
   user of a Free build cannot pick an icon that would show as the placeholder. The web-configurator has its own
   list, deliberately kept as a separate copy for simplicity; the two agree on names as long as both follow Font
   Awesome's canonical names.

The names are the contract between the clients and the stored configuration: a name must never change meaning, and a
name that was ever offered must stay in the mapping, because it may be stored in a user's configuration.

## The files

| File                                        | What it is                                                                     | Maintained by                                  |
| ------------------------------------------- | ------------------------------------------------------------------------------ | ---------------------------------------------- |
| `resources/icons/icon-font.ttf`             | The embedded font: Font Awesome **Free** Solid, patched (see below)            | `tools/icon-font.sh --free`, never by hand     |
| `resources/icons/icon-font.json`            | Provenance of the embedded font: edition, release, checksums                   | written with the font                          |
| `resources/icons/icon-mapping.json`         | `uc:` name → code point, every icon of the Font Awesome release + the overrides | `tools/icon-font.py mapping`, never by hand    |
| `resources/icons/icon-mapping-overrides.json` | Names that are not Font Awesome names, each mapped to the Font Awesome icon it shows | by hand, rarely                           |
| `resources/icons/icon-fallback.json`        | Pro-only name → Free name to draw instead, and the placeholder                 | by hand, when the UI starts using a Pro-only icon |
| `resources/icons/LICENSE-fontawesome.txt`   | Licence of the embedded font (SIL OFL 1.1)                                     | with a Font Awesome upgrade                    |
| `tools/icon-font.sh`, `tools/icon-font.py`  | Build, patch, generate and verify all of the above; the release is pinned in the shell script | —                              |

### The mapping and the overrides

`icon-mapping.json` is generated from the Font Awesome package: every **canonical** icon name of the release, all
families and editions (Pro included, brands included), mapped to its code point. Font Awesome's aliases
(`home` for `house`, `chain` for `link`) are *not* mapped — only one name per icon, so that the icon selector and the
web-configurator list the same names.

`icon-mapping-overrides.json` holds the 82 names on top of that, each pointing at the Font Awesome name whose glyph it
shows. They exist for one reason: the remote had its own icon names before it used Font Awesome (`activity`, `apps`,
`bw`, `ff`, `lamp-1`, `wifi-01`, `profile-hat`, …), those names are stored in existing configurations, and five Font
Awesome names (`heat`, `info`, `link`, `list`, `square-full`) are deliberately pointed at a different icon than Font
Awesome's own (`fire`, `circle-info`, `link-horizontal`, `square-list`, `square`) for the same reason. The overrides
name the target icon, not a code point, so they survive a release upgrade unchanged. Add an override only for a name
that already exists in configurations out there; a new icon in the UI simply uses its Font Awesome name.

Both Font Awesome editions give an icon the same code point in every style, which is why one mapping serves both
fonts. The Free edition draws about 1400 of the mapped names, the Pro edition about 3400; the rest are brands, which
neither embedded font contains.

`tools/icon-font.py check-mapping` (run by CI) verifies that the overrides are applied, that every `uc:` name the
sources use is in the mapping, that every fallback target and the placeholder are drawable by the embedded Free
font, and that every Pro-only name the sources use has a fallback entry.

## The two editions

|          | Free (built into every binary)                                 | Pro (file installed by the firmware)                     |
| -------- | -------------------------------------------------------------- | -------------------------------------------------------- |
| Source   | `@fortawesome/fontawesome-free`, redistributable (SIL OFL 1.1) | `@fortawesome/fontawesome-pro`, commercially licensed    |
| Style    | Solid, the only full weight the Free edition has               | Light, the design weight of the UI                       |
| Icons    | about 1400 of the mapped icon names resolve                    | about 3400 resolve                                       |
| Runtime  | missing icons are replaced through `icon-fallback.json`        | every mapped icon exists, the fallback never applies     |
| Where    | `resources/icons/icon-font.ttf`, compiled into `remote-ui`     | a file on the device, named in `UC_ICON_FONT_PATH`       |

- **The Pro font must never be committed**, and it is never built into the binary either. The device build,
  the desktop build and a build from the public sources all carry the same Free font.
- The tracked font is the Free edition, and CI fails if that changes.

### How the app picks the font at start-up

1. If `UC_ICON_FONT_PATH` names a readable file and Qt can load it, that font is used.
2. Otherwise the embedded Free font is used, with a warning in the log when a path was set but did not
   work (missing file, unreadable, not a font).

The log line `Icon font loaded: <family> from <path>` says which one is in use.

On the device the firmware sets `UC_ICON_FONT_PATH` in the `remote-ui` service and installs the Pro file
readable by the `remote-ui` user only. A custom UI build installed by a user runs in a separate sandbox
where that file is not available, so it renders with the embedded Free font.

On a desktop, a developer with a Font Awesome Pro subscription can build the Pro file (below) and set
`UC_ICON_FONT_PATH` in the run environment to see the device rendering; without it the simulator shows the
Free rendering, which is also what CI and the unit tests use.

### What is embedded, and why it is patched

`tools/icon-font.py build` takes a Font Awesome webfont and patches it in two ways:

- **Emoji code points are unmapped.**  
  Font Awesome maps several hundred emoji code points to icon glyphs, so any text containing an emoji would render as
  icons instead of the emoji. Every emoji is drawn by Google's Noto Color Emoji font, which the device firmware
  installs; the icon font never draws one (ADR 0010).
- **The font is renamed** to the family `UC Icons`.  
  Font Awesome Free is SIL OFL 1.1 with the reserved font name "Font Awesome", which a modified version must not carry.
  Renaming both editions to the same family also keeps the QML free of edition-specific names.  
  The copyright, version and license entries of the font are left untouched, as the licence requires,
  and `resources/icons/LICENSE-fontawesome.txt` ships next to the font.

`resources/icons/icon-font.json` records what the embedded font is: edition, Font Awesome release, the source file with
its checksum, how many emoji code points were unmapped, and the checksum of the result. A Pro file built for the
firmware gets the same provenance file next to it. `tools/icon-font.py check` verifies the tracked font against its
provenance, which is what CI runs.

## Adding an icon to the UI

1. Pick the Font Awesome name (the canonical one, e.g. `house`, not the alias `home`). It is already in the mapping.
2. Use it as `uc:<name>` in QML or C++.
3. Run the app once with the Free font (the default on a desktop) and look for `Icon <name> not in the icon font` in
   the log. If it appears, the icon exists only in Pro: add a `<name>: <free-name>` entry to `icon-fallback.json`,
   choosing the closest icon the Free font can draw (`tools/icon-font.py check-mapping` tells you when a used
   Pro-only name has no fallback). Users of the device never see the fallback; builds from the public sources do.

The fallback is looked up by the name as it is stored, not by the icon it shows: an override name (`switch` for
`light-switch`, `integration` for `puzzle`) needs its own entry, even when its target has one.

Icons the core sends are not in the UI sources, so `check-mapping` cannot see them: the default icon of every entity
type, the media browser thumbnails (`icon://uc:...`), the button pages of IR remotes and Bluetooth peripherals, the
default integration icon and the default activity group. `testIconFont` keeps their list (taken from remote-core) and
fails when one of them would be drawn as the placeholder by the Free font; extend it when the core adds a default.

## Rebuilding the font and the mapping

Both editions and the mapping are produced by the same tools; nothing has to be run for a normal build.

```shell
pip install fonttools

# Free, e.g. after a Font Awesome upgrade. Writes the tracked font. Commit the result.
tools/icon-font.sh --free
tools/icon-font.sh --free /ci-cache/fontawesome-free

# Pro, on the firmware build system. Writes the font and its provenance to the given path,
# never into the repository. Install the file on the device and set UC_ICON_FONT_PATH.
FONTAWESOME_NPM_AUTH_TOKEN=<token> tools/icon-font.sh --pro --output /out/icon-font.ttf
tools/icon-font.sh --pro --output /out/icon-font.ttf /ci-cache/fontawesome-pro

# The mapping, from an extracted Pro package (the Free package lists only the Free icons).
tools/icon-font.py mapping /ci-cache/fontawesome-pro
tools/icon-font.py check-mapping
```

With a package directory the shell script uses it as it is; without one it downloads the package with `npm pack`.  
For Pro that needs the subscription token, either in `FONTAWESOME_NPM_AUTH_TOKEN` or in the `@fortawesome` registry
configuration. The token is written to a temporary `.npmrc` and never persisted. `--pro` refuses to write into
`resources/`, so a Pro build cannot end up in the working tree by accident.

The mapping generator reads `metadata/icon-families.json` of the package when it has it, otherwise
`scss/_variables.scss` (a trimmed package copy still has that); both give the same result.

### Upgrading Font Awesome

The release is pinned once, in `FA_VERSION` of `tools/icon-font.sh`, and the font, the mapping and the firmware's Pro
file must all come from that release: a release adds icons, occasionally redraws glyphs, and the script refuses a
package of another release.

1. Compare the releases first: new icons are harmless, a removed or renamed icon would break stored configurations
   (an override can keep the old name alive), and a redrawn glyph changes what users see on their tiles. Font Awesome
   has not removed a name in 6.x so far. Rasterising the two `fa-light-300.ttf` files with fontTools and Pillow shows
   which glyphs actually changed; the changelog names the ones that matter.
2. Bump `FA_VERSION`, run `tools/icon-font.sh --free <package>` and `tools/icon-font.py mapping <pro-package>`,
   then `check-mapping` and `make test` (`testIconFont`).
3. Write the `CHANGELOG.md` entry: the new icons and the redrawn ones.
4. The firmware builds its Pro file from the same release; bump both together.

## CI

The code-guidelines workflow runs `tools/icon-font.py check --require-free` and `check-mapping`. The font check fails:

- if the font does not match its provenance file.
- if the provenance says the Pro edition.
- if the font still carries the Font Awesome family name.

The build workflow needs neither a subscription token nor `fonttools`: every job builds a binary
with the embedded Free font. Whether the firmware that ships actually installed the licensed font is
checked by the firmware build, which is where the two are brought together.

The unit tests run with the Free font: `testIconFont` asserts the fallback behaviour, which a Pro font does
not exhibit, and that an external font is only used when `UC_ICON_FONT_PATH` names a file that loads.
