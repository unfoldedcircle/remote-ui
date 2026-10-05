## MODIFIED Requirements

### Requirement: Icon identifiers
An icon SHALL be referenced by a string `<prefix>:<name>`, where only the text between the first and the second colon is used as the name. A `uc:` icon (any prefix containing `uc`) SHALL resolve to a glyph of the icon font through the embedded name mapping, which SHALL be generated from the Font Awesome release the font is built from: every canonical Font Awesome icon name of that release, all editions, mapped to its code point (aliases are not mapped), plus the names of the tracked overrides file — the remote's own pre-Font-Awesome icon names such as `remote`, `blind`, `activity` or `lamp-1`, and the Font Awesome names the UI deliberately points at another icon (`heat`, `info`, `link`, `list`, `square-full`) — each mapped to the Font Awesome icon it shows. At Font Awesome 6.7.2 the mapping SHALL have 3891 names: 3814 Font Awesome icons and 82 overrides, 5 of which replace a Font Awesome name. A name once offered SHALL stay in the mapping, because it may be stored in a configuration. That mapping SHALL be the same for both editions of the icon font; whether the loaded font can actually draw a mapped name depends on the edition, and a name it cannot draw is replaced as specified in "Missing and default icons". The repository check SHALL fail if an override is not applied, if a `uc:` name used by the sources is not in the mapping, if a fallback target or the placeholder is not drawable by the Free font, or if a Pro-only name used by the sources has no fallback. A `custom:` icon (any prefix containing `custom`) SHALL resolve to the file `<UC_RESOURCE_PATH>/Icon/<name>`. An identifier containing `ctv:` SHALL resolve to the TV channel icon file `<UC_RESOURCE_PATH>/TvChannelIcon/<name>`. A trailing `.png`, `.jpg` or `.jpeg` SHALL be kept as part of the file name.

#### Scenario: Built-in icon
- **WHEN** a tile shows the icon `uc:lightbulb`
- **THEN** the lightbulb glyph of the icon font is drawn

#### Scenario: Same names in both editions
- **WHEN** the same page is shown on a device build and on a public build
- **THEN** the icon identifiers are the same in both and only the drawn glyph may differ

#### Scenario: Legacy name
- **WHEN** a configuration made before the Font Awesome icon set uses `uc:lamp-1`
- **THEN** the `lamp-floor` glyph is drawn, because the overrides file maps the old name to it

#### Scenario: Alias is not a name
- **WHEN** an entity uses `uc:car-people`, a Font Awesome alias of `carpool`, and no override carries that alias
- **THEN** the icon is empty, because only canonical names are mapped; `uc:carpool` draws it

#### Scenario: Custom icon
- **WHEN** a tile shows the icon `custom:sofa.png` and `<UC_RESOURCE_PATH>/Icon/sofa.png` exists
- **THEN** that image file is drawn

#### Scenario: TV channel icon
- **WHEN** a tile shows the icon `ctv:bbc1.png` and `<UC_RESOURCE_PATH>/TvChannelIcon/bbc1.png` exists
- **THEN** that image file is drawn

### Requirement: Icon font edition
The icon font SHALL exist in two editions that differ only in which of the mapped icon names they can draw and in the drawing weight: the redistributable **Free** edition, which SHALL be the one kept in the repository and embedded in every binary — every developer build, every unit test, every CI job, every release artefact and every build from the published sources — and the licensed **Pro** edition, which SHALL never be committed to any repository and never be built into a binary; the firmware SHALL install it as a file on the device and name it in `UC_ICON_FONT_PATH`. Both editions and the mapping SHALL come from one pinned Font Awesome release, 6.7.2. At start-up the UI SHALL load the file named in `UC_ICON_FONT_PATH` when the variable is set and the file exists, is readable and loads as a font, and the embedded Free edition otherwise, logging a warning with the path when a path was set but could not be used; exactly one icon font SHALL be registered. Both editions SHALL carry the same font family name, so that no part of the UI names an edition. About 1400 of the 3891 mapped icon names SHALL resolve in the Free edition and about 3400 in the Pro edition, the rest being brand icons that neither font contains; a mapped name the loaded edition cannot draw is replaced as specified in "Missing and default icons". The repository check SHALL fail if the icon font kept in the repository is not the Free edition, does not match its recorded provenance, or still carries its original vendor family name.

#### Scenario: Device build
- **WHEN** the UI starts on a device whose firmware installed the licensed edition and set `UC_ICON_FONT_PATH` to it
- **THEN** that file is loaded, the log names its path, every mapped icon name renders as its own icon in the Light weight, and neither a fallback nor the placeholder is ever drawn

#### Scenario: Public build
- **WHEN** the UI starts without `UC_ICON_FONT_PATH` — a desktop build, a unit test, a CI job, a build from the published sources, or a custom build installed by a user, which runs in a sandbox where the firmware's file is not available
- **THEN** the embedded Free edition is used, the icons render in its weight, and the names only the Pro edition has render as their fallback icon or as the placeholder

#### Scenario: Override that cannot be used
- **WHEN** `UC_ICON_FONT_PATH` names a file that does not exist, is not readable or is not a font
- **THEN** a warning naming the path is logged, the embedded Free edition is used, and the UI starts normally

#### Scenario: Licensed font committed by mistake
- **WHEN** the icon font kept in the repository is replaced by the licensed edition
- **THEN** the repository check fails and the change cannot be merged

#### Scenario: New Pro-only icon used in the UI
- **WHEN** a change starts using an icon name that only the Pro edition has and adds no fallback entry for it
- **THEN** the repository check fails, and a build running without the licensed file would draw the placeholder there and log the missing fallback

#### Scenario: Release upgrade
- **WHEN** the pinned Font Awesome release is changed
- **THEN** the Free font, the mapping and the firmware's Pro file are rebuilt from the new release together, and the changelog names the icons that were added and the glyphs that were redrawn
