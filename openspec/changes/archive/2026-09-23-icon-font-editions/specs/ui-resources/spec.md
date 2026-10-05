## MODIFIED Requirements

### Requirement: Icon identifiers
An icon SHALL be referenced by a string `<prefix>:<name>`, where only the text between the first and the second colon is used as the name. A `uc:` icon (any prefix containing `uc`) SHALL resolve to a glyph of the embedded icon font through the embedded name mapping of 3877 names (Font Awesome 6 names plus Unfolded Circle additions such as `remote`, `blind` or `activity`). That mapping SHALL be the same for both editions of the icon font; whether the embedded font can actually draw a mapped name depends on the edition, and a name it cannot draw is replaced as specified in "Missing and default icons". A `custom:` icon (any prefix containing `custom`) SHALL resolve to the file `<UC_RESOURCE_PATH>/Icon/<name>`. An identifier containing `ctv:` SHALL resolve to the TV channel icon file `<UC_RESOURCE_PATH>/TvChannelIcon/<name>`. A trailing `.png`, `.jpg` or `.jpeg` SHALL be kept as part of the file name.

#### Scenario: Built-in icon
- **WHEN** a tile shows the icon `uc:lightbulb`
- **THEN** the lightbulb glyph of the icon font is drawn

#### Scenario: Same names in both editions
- **WHEN** the same page is shown on a device build and on a public build
- **THEN** the icon identifiers are the same in both and only the drawn glyph may differ

#### Scenario: Custom icon
- **WHEN** a tile shows the icon `custom:sofa.png` and `<UC_RESOURCE_PATH>/Icon/sofa.png` exists
- **THEN** that image file is drawn

#### Scenario: TV channel icon
- **WHEN** a tile shows the icon `ctv:bbc1.png` and `<UC_RESOURCE_PATH>/TvChannelIcon/bbc1.png` exists
- **THEN** that image file is drawn

### Requirement: Missing and default icons
An icon that cannot be resolved SHALL leave its area empty and SHALL only be logged: an identifier without a colon, an unknown prefix, or a custom or TV channel icon whose file does not exist. An element with no icon identifier at all SHALL leave its area empty without asking for a lookup and without logging anything, because having no icon is a normal state. A `uc:` name that is not in the mapping SHALL leave its area empty and SHALL NOT be looked up as a TV channel icon file. A `uc:` name that is in the mapping but that the embedded icon font cannot draw SHALL be replaced by the alternative name listed for it in the embedded fallback mapping and, when it has no entry there or the alternative cannot be drawn either, by the placeholder icon, a question mark in a circle; every replacement SHALL be logged with the name that was requested. An entity without an icon SHALL get a default icon by type: button and switch `uc:power-on`, climate `uc:climate`, cover `uc:blind`, light `uc:light`, media player `uc:music`, sensor `uc:sensor`, remote `uc:remote`, activity and macro `uc:activity`, voice assistant `uc:microphone`, select `uc:list-dropdown`, any other type `uc:warning`.

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

### Requirement: Icon rendering
A glyph icon SHALL be drawn centred in its square box in the icon font at a pixel size of half the box size (rounded), off-white unless a colour is given, with native text rendering. An image icon SHALL be decoded asynchronously at the box size, scaled to fit the box with its aspect ratio preserved, and kept in the image cache. The UI SHALL NOT name the icon font family itself: it SHALL take the family from the icon font it loaded at startup, which is the same for both editions. If that font cannot be loaded the UI SHALL log "Icon font failed to load" and continue with the fonts the platform offers.

#### Scenario: Glyph size
- **WHEN** an icon is shown in an 80 px box
- **THEN** the glyph is drawn at 40 px

#### Scenario: Non-square custom image
- **WHEN** a 200 x 100 custom icon is shown in an 80 px box
- **THEN** it is drawn 80 x 40, centred

#### Scenario: Font family taken from the loaded font
- **WHEN** the embedded icon font is exchanged for the other edition before a build
- **THEN** the icons still render, because no screen names an icon font family

### Requirement: Icon selector
The icon selector SHALL be a full-screen overlay titled "Select icon" with two tabs, "Unfolded Icons" and "Custom Icons", a grid of 120 x 120 px cells each showing a 100 px icon, and a "Close" button. The Unfolded tab SHALL list as `uc:<name>`, sorted by name, only those names of the icon font mapping that the embedded icon font can actually draw, so that no offered icon would show as a fallback or as the placeholder; the Custom tab SHALL list every `.png`, `.jpg` and `.jpeg` file (extension matched case-insensitively) in `<UC_RESOURCE_PATH>/Icon/` as `custom:<file name>`, sorted by name ignoring case. Both lists SHALL be read when the selector is created, not each time it opens. There SHALL be no search field and no swiping between tabs. Tapping an icon SHALL select it and close the overlay; the selected identifier is applied to the entity (from the page edit menu) or the profile (from the profile menu). Opening SHALL start on the Unfolded tab with its first icon.

#### Scenario: Selecting an icon
- **WHEN** the user taps the `uc:tv` cell
- **THEN** the overlay closes and `uc:tv` becomes the icon of the entity or profile being edited

#### Scenario: No resource directory
- **WHEN** `UC_RESOURCE_PATH` is unset or its `Icon` folder is empty
- **THEN** the Custom Icons tab shows an empty grid

#### Scenario: Icon the embedded font cannot draw
- **WHEN** the user opens the selector in a build whose icon font is the Free edition
- **THEN** the Pro-only names are missing from the grid and cannot be picked for an entity or a profile

#### Scenario: Icon uploaded while the UI runs
- **WHEN** a new custom icon file is added after the selector was created
- **THEN** it does not appear in the Custom Icons tab until the screen hosting the selector is created again

### Requirement: Embedded assets and fonts
The binary SHALL embed the icon font, its name mapping, its fallback mapping, the dock pictures (Dock 2; Dock 3 dark and silver, each charging and not charging), a small loading image, the quarter-arc image of the startup animation and the keypad picture of the button simulator. Text SHALL use the font families "Poppins" (primary, default 30 px) and "Space Mono" (secondary, default 24 px), each also offered with the first letter of every word or all letters capitalised, and the status bar clock SHALL use Poppins 24 px with 110 % letter spacing. These text fonts SHALL NOT be embedded: they are taken from the fonts installed on the system (the device image, or e.g. `~/.fonts` on a desktop), and Qt's default font is used when they are missing. Custom icons, TV channel icons, background images, sounds, sound effects and legal documents SHALL only come from the file system.

#### Scenario: Desktop without Poppins
- **WHEN** the simulator runs on a desktop where Poppins and Space Mono are not installed
- **THEN** all text is drawn in the system's fallback font while icons still render from the embedded icon font

#### Scenario: Startup log
- **WHEN** the icon font loads successfully at startup
- **THEN** "Icon font loaded:" and the family the font registered are logged

## ADDED Requirements

### Requirement: Icon font edition
The embedded icon font SHALL exist in two editions that differ only in which of the mapped icon names they can draw and in the drawing weight: the redistributable **Free** edition, which is the one kept in the repository and therefore the one used by every developer build, every unit test, every pull-request build and every publicly released source tree, and the licensed **Pro** edition, which SHALL be overlaid only by the build that produces the shipping device binary and SHALL never be committed to any repository. Both editions SHALL carry the same font family name, so that no part of the UI names an edition. About 1450 of the 3877 mapped icon names SHALL resolve in the Free edition and about 3300 in the Pro edition; a mapped name the embedded edition cannot draw is replaced as specified in "Missing and default icons". The build SHALL record which edition it embedded, and the repository check SHALL fail if the icon font kept in the repository is not the Free edition, does not match its recorded provenance, or still carries its original vendor family name.

#### Scenario: Device build
- **WHEN** the device binary is built with the icon font subscription token available
- **THEN** it embeds the Pro edition, every mapped icon name renders as its own icon, and neither a fallback nor the placeholder is ever drawn

#### Scenario: Public build
- **WHEN** the same sources are built without that token, for example from the public repository or from a fork
- **THEN** the build succeeds with the Free edition, the icons render in its weight, and the names only the Pro edition has render as their fallback icon or as the placeholder

#### Scenario: Licensed font committed by mistake
- **WHEN** the icon font kept in the repository is replaced by the licensed edition
- **THEN** the repository check fails and the change cannot be merged

#### Scenario: New Pro-only icon used in the UI
- **WHEN** a change starts using an icon name that only the Pro edition has and adds no fallback entry for it
- **THEN** a public build draws the placeholder there and logs the missing fallback, which is how the gap is found
