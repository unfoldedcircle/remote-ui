## MODIFIED Requirements

### Requirement: Icon identifiers
An icon SHALL be referenced by a string `<prefix>:<name>`, where only the text between the first and the second colon is used as the name. A `uc:` icon (any prefix containing `uc`) SHALL resolve to a glyph of the icon font through the embedded name mapping, which SHALL be generated from the Font Awesome release the font is built from: every canonical Font Awesome icon name of that release, all editions, mapped to its code point (aliases are not mapped), plus the names of the tracked overrides file — the remote's own pre-Font-Awesome icon names such as `remote`, `blind`, `activity` or `lamp-1`, and the Font Awesome names the UI deliberately points at another icon (`heat`, `info`, `link`, `list`, `square-full`) — each mapped to the Font Awesome icon it shows. At Font Awesome 6.7.2 the mapping SHALL have 3891 names: 3814 Font Awesome icons and 82 overrides, 5 of which replace a Font Awesome name. A name once offered SHALL stay in the mapping, because it may be stored in a configuration. That mapping SHALL be the same for both editions of the icon font; whether the loaded font can actually draw a mapped name depends on the edition, and a name it cannot draw is replaced as specified in "Missing and default icons". The repository check SHALL fail if an override is not applied, if a `uc:` name used by the sources is not in the mapping, if a fallback target or the placeholder is not drawable by the Free font, or if a Pro-only name used by the sources has no fallback. Because the icon names the core assigns on its own appear in no UI source, the unit tests SHALL keep the list of them, taken from the core, and SHALL fail when the Free font would draw one of them as the placeholder; the list SHALL be extended when the core adds such a name. A `custom:` icon (any prefix containing `custom`) SHALL resolve to the file `<UC_RESOURCE_PATH>/Icon/<name>`. An identifier containing `ctv:` SHALL resolve to the TV channel icon file `<UC_RESOURCE_PATH>/TvChannelIcon/<name>`. A trailing `.png`, `.jpg` or `.jpeg` SHALL be kept as part of the file name.

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

#### Scenario: New default icon of the core
- **WHEN** the core starts assigning a Pro-only icon name on its own and that name is added to the list kept by the unit tests without a fallback entry
- **THEN** the unit tests fail until a fallback is added

### Requirement: Missing and default icons
An icon that cannot be resolved SHALL leave its area empty and SHALL only be logged: an identifier without a colon, an unknown prefix, or a custom or TV channel icon whose file does not exist. An element with no icon identifier at all SHALL leave its area empty without asking for a lookup and without logging anything, because having no icon is a normal state. A `uc:` name that is not in the mapping SHALL leave its area empty and SHALL NOT be looked up as a TV channel icon file. Whether the icon font can draw a mapped name SHALL be decided by the loaded icon font alone: a code point that only another installed font has — a system symbol or text font that Qt would fall back to — SHALL count as missing, so such a font's unrelated glyph is never drawn in place of an icon. A `uc:` name that is in the mapping but that the loaded icon font cannot draw SHALL be replaced by the alternative name listed for it in the embedded fallback mapping and, when it has no entry there or the alternative cannot be drawn either, by the placeholder icon, a question mark in a circle; every replacement SHALL be logged with the name that was requested. The fallback SHALL be looked up by the name as it is stored, not by the icon it shows, so an override name (`switch`, `integration`, `playlist`) SHALL have its own fallback entry even when the icon it points at has one. The icon names the core assigns on its own SHALL have a fallback the Free edition can draw when the Free edition lacks them: the default icons of the entity types, among them those of macro, select, switch and IR emitter entities; the media browser thumbnails without an image (`icon://uc:…`), such as artists, tracks, playlists, channels, apps and web links; the button pages of IR remotes and Bluetooth peripherals, such as home, play/pause and record; the icon of an integration without one; and the default activity group. An entity without an icon SHALL get a default icon by type: button and switch `uc:power-on`, climate `uc:climate`, cover `uc:blind`, light `uc:light`, media player `uc:music`, sensor `uc:sensor`, remote `uc:remote`, activity and macro `uc:activity`, voice assistant `uc:microphone`, select `uc:list-dropdown`, any other type `uc:warning`.

#### Scenario: Custom icon file deleted
- **WHEN** an entity uses `custom:old.png` and the file no longer exists
- **THEN** the tile shows no icon and no placeholder

#### Scenario: Unknown built-in name
- **WHEN** an entity uses `uc:does-not-exist`
- **THEN** the icon is empty, even when a TV channel icon file of that name exists

#### Scenario: Icon the embedded font cannot draw
- **WHEN** a build whose icon font is the Free edition shows `uc:keyboard-down`, a name only the Pro edition has
- **THEN** the `keyboard` icon named for it in the fallback mapping is drawn and the substitution is logged

#### Scenario: No fallback entry
- **WHEN** such a build shows a Pro-only icon name that has no entry in the fallback mapping
- **THEN** the placeholder, a question mark in a circle, is drawn and the missing fallback is logged

#### Scenario: Icon the embedded font can draw
- **WHEN** a build whose icon font is the Pro edition shows the same `uc:keyboard-down`
- **THEN** that icon itself is drawn and no fallback is applied

#### Scenario: Entity without icon
- **WHEN** the core delivers a light entity without an icon
- **THEN** the light shows `uc:light`

#### Scenario: Element with no icon identifier
- **WHEN** a screen element is shown whose icon identifier is empty
- **THEN** its icon area stays empty and nothing is logged

#### Scenario: Another font has a glyph at the code point
- **WHEN** a build with the Free edition shows `uc:rec`, which the Free edition lacks, on a system where a symbol font installed next to it has a character at that code point
- **THEN** the fallback `circle` of the icon font is drawn, not the other font's character

#### Scenario: Default icon assigned by the core
- **WHEN** a build with the Free edition shows a macro entity with the core's default icon `uc:macro`, or a select entity with `uc:list-dropdown`
- **THEN** their fallbacks `list-ol` and `square-caret-down` are drawn instead of the placeholder

#### Scenario: Override name with its own fallback
- **WHEN** a build with the Free edition shows `uc:switch`, an override name that points at the Pro-only `light-switch`
- **THEN** the fallback listed for `switch` itself is drawn, because the fallback of `light-switch` is not consulted for it

### Requirement: Icon selector
The icon selector SHALL be a full-screen overlay titled "Select icon" with two tabs, "Unfolded Icons" and "Custom Icons", a grid of 120 x 120 px cells each showing a 100 px icon, and a "Close" button. The Unfolded tab SHALL list as `uc:<name>`, sorted by name, only those names of the icon font mapping that the loaded icon font itself can actually draw — a code point only another installed font has does not count — so that no offered icon would show as a fallback, as the placeholder or as another font's character; the Custom tab SHALL list every `.png`, `.jpg` and `.jpeg` file (extension matched case-insensitively) in `<UC_RESOURCE_PATH>/Icon/` as `custom:<file name>`, sorted by name ignoring case. Both lists SHALL be read when the selector is created, not each time it opens. There SHALL be no search field and no swiping between tabs. Tapping an icon SHALL select it and close the overlay; the selected identifier is applied to the entity (from the page edit menu) or the profile (from the profile menu). Opening SHALL start on the Unfolded tab with its first icon.

#### Scenario: Selecting an icon
- **WHEN** the user taps the `uc:tv` cell
- **THEN** the overlay closes and `uc:tv` becomes the icon of the entity or profile being edited

#### Scenario: No resource directory
- **WHEN** `UC_RESOURCE_PATH` is unset or its `Icon` folder is empty
- **THEN** the Custom Icons tab shows an empty grid

#### Scenario: Icon the embedded font cannot draw
- **WHEN** the user opens the selector in a build whose icon font is the Free edition
- **THEN** the Pro-only names are missing from the grid and cannot be picked for an entity or a profile, also those for which another installed font has a character at the same code point

#### Scenario: Icon uploaded while the UI runs
- **WHEN** a new custom icon file is added after the selector was created
- **THEN** it does not appear in the Custom Icons tab until the screen hosting the selector is created again
