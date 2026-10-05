# ui-resources Specification

## Purpose

The resources the UI renders: built-in and custom icons, the icon selector, background images and sounds, media image loading, legal documents, QR codes and embedded fonts.

## Requirements

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

### Requirement: Icon selector keypad navigation
The icon selector SHALL own the keypad while open. In the grid DPAD_LEFT / DPAD_RIGHT SHALL move by one cell and DPAD_UP / DPAD_DOWN by one row (as many cells as fit in a row, 4 on a 480 px screen), without wrapping. DPAD_UP from the first row SHALL move to the tabs, where DPAD_LEFT selects "Unfolded Icons" and DPAD_RIGHT "Custom Icons", each time resetting that tab's grid to its first icon; DPAD_DOWN from the tabs SHALL return to the grid, or to Close when the grid is empty. DPAD_DOWN below the last row SHALL move to Close; DPAD_UP from Close returns to the grid, or to the tabs when it is empty. DPAD_MIDDLE SHALL select the current icon in the grid and close the overlay on Close; it does nothing on the tabs. BACK and HOME SHALL close the overlay without a selection. The current cell, tab or Close button SHALL be outlined only while the keypad is in use.

#### Scenario: Walking to the tabs
- **WHEN** the selection is on the second icon and DPAD_UP is pressed
- **THEN** the tabs are selected and DPAD_RIGHT switches to Custom Icons with its first icon current

#### Scenario: Leaving the grid downwards
- **WHEN** the selection is in the last row and DPAD_DOWN is pressed
- **THEN** Close is selected and DPAD_MIDDLE closes the overlay without changing the icon

### Requirement: Resource files from the resource directory
TV channel icons, page background images and sounds SHALL be read at runtime from the directory given by `UC_RESOURCE_PATH`, in the sub-folders `TvChannelIcon`, `BackgroundImage` and `Sound`. A background image or sound identifier `<prefix>:<file>` SHALL resolve to `<sub-folder>/<file>` for any prefix; an empty file name, an identifier without a colon or a missing file SHALL resolve to nothing. A page background image SHALL be decoded asynchronously at the header size, cropped to fill it and cached.

#### Scenario: Page with background image
- **WHEN** a page's image is `custom:living.jpg` and `<UC_RESOURCE_PATH>/BackgroundImage/living.jpg` exists
- **THEN** the page header shows the image cropped to the header

#### Scenario: Missing background image
- **WHEN** the image file does not exist
- **THEN** the page header shows no image

### Requirement: Media image provider
Media artwork downloaded for a media player (see `media-player` for download and cache size) SHALL be served to the screens under `image://media-art/<key>`, where the key is the 40-character lowercase hex SHA-1 of `<entity id>:<download request number>`, so every stored image gets a new URL. The provider SHALL return the stored image unchanged when it fits the requested size and a smooth, aspect-preserving downscale otherwise; it SHALL never upscale. A key that is not, or no longer, stored SHALL produce an image load error.

#### Scenario: Downscaled delivery
- **WHEN** a stored 1024 x 768 artwork is displayed in a 480 x 420 area
- **THEN** it is delivered at 480 x 360

#### Scenario: Evicted image
- **WHEN** a screen still references an artwork URL whose image was evicted or replaced
- **THEN** loading that URL fails and the image loading failure behaviour applies

### Requirement: Remote image loading
Media artwork and media browser thumbnails SHALL be loaded asynchronously at their display size. HTTP(S) images SHALL use the image cache; `data:` and `image://media-art/` images SHALL not be cached by the view. A loading indicator (a dot pulsing between 20 px and 2 px, 600 ms each way with a 300 ms pause) SHALL appear only when loading takes longer than 500 ms. A loaded image SHALL fade in over 800 ms. A failed load SHALL be retried 2 times, 1 s apart; after the third failure the image SHALL be hidden and marked as failed. An empty URL SHALL clear the image.

#### Scenario: Slow image
- **WHEN** a thumbnail takes 2 s to load
- **THEN** the pulsing dot appears after 500 ms and disappears when the image fades in

#### Scenario: Broken URL
- **WHEN** an image URL fails three times
- **THEN** no image is shown and no further attempts are made until the URL changes

### Requirement: Legal documents
The About entries Regulatory, Terms & conditions and Warranty information SHALL show the alphabetically first file (ignoring case) ending in `.html` or `.md` in `<UC_LEGAL_PATH>/regulatory`, `<UC_LEGAL_PATH>/terms` and `<UC_LEGAL_PATH>/warranty` respectively, rendered as rich text regardless of the extension, word-wrapped in the 24 px secondary font, with the document's folder as base for relative references such as images. The Regulatory entry SHALL be offered on every model. The onboarding Terms step SHALL show the same Terms document in its popup. A missing folder or file SHALL show an empty page. DPAD_DOWN / DPAD_UP SHALL scroll by 100 px per press with a 500 ms animation, clamped to the content, repeating while the key is held.

#### Scenario: Terms document present
- **WHEN** `<UC_LEGAL_PATH>/terms` contains `terms_en.html` and `terms_de.html`
- **THEN** "Terms & conditions" shows `terms_de.html`

#### Scenario: Markdown file on a document page
- **WHEN** the only file in `<UC_LEGAL_PATH>/warranty` is `warranty.md`
- **THEN** its content is shown as rich text, so Markdown syntax is not formatted

#### Scenario: Legal path not set
- **WHEN** `UC_LEGAL_PATH` is unset
- **THEN** all four legal pages are empty

### Requirement: Licenses document
The Licenses entry SHALL show `<UC_LEGAL_PATH>/licenses/README.md` as Markdown, split into sections at every line starting with `## `, each section rendered as its own block of a scrolling list in the 24 px secondary font. DPAD_DOWN / DPAD_UP SHALL scroll it by 100 px per press as on the other legal pages.

#### Scenario: License overview
- **WHEN** the README contains an introduction and three `## ` headings
- **THEN** the page shows four blocks, the headings formatted as Markdown headings

### Requirement: Links inside legal documents
Activating a link whose target contains `http` SHALL do nothing. The first other link on a legal page SHALL replace the page content with the linked file, read relative to the document's folder, as rich text; on the Licenses page each line of the linked file becomes its own block. Every further link on that page SHALL be ignored until the page is opened again. A link to a file that cannot be read SHALL leave the page empty. A link SHALL only be followed when it is a relative reference that stays inside the legal directory (`UC_LEGAL_PATH`): a link with any URL scheme (`http`, `https`, `ftp`, `mailto`, `file`, …), a protocol-relative link (`//host/…`), an absolute path and a path that leaves the legal directory through `..` SHALL be refused, logged and treated like a file that cannot be read. Nothing SHALL ever be fetched from the network. The linked file SHALL be found independently of the working directory of the app, and relative references inside it, such as images, SHALL be resolved against the linked file's own folder.

#### Scenario: Following a license link
- **WHEN** the user taps a link to `qt/LICENSE.txt` in the licenses README
- **THEN** the page shows `<UC_LEGAL_PATH>/licenses/qt/LICENSE.txt`
- **AND** tapping a link inside that text does nothing

#### Scenario: Web link
- **WHEN** the user taps a link to `https://unfoldedcircle.com`
- **THEN** nothing happens

#### Scenario: Non-file link
- **WHEN** the user taps a `mailto:` link in a legal document
- **THEN** the page content becomes empty

#### Scenario: App not started from the root directory
- **WHEN** the app runs with a working directory other than `/`, as on a desktop, and the user taps a link to another document of the legal directory
- **THEN** the linked document is shown, and the images it references from its own folder are drawn

#### Scenario: Link leading out of the legal directory
- **WHEN** a legal document links to `../../etc/passwd`, `/etc/passwd`, `file:///etc/passwd` or `//example.com/x`
- **THEN** the link is not followed, a warning is logged and the page content becomes empty

### Requirement: QR codes
The UI SHALL generate QR codes itself from the UTF-8 text, with error correction level Low (raised to a higher level when the text still fits the same symbol version), the smallest symbol version that fits, an automatically chosen mask and no quiet zone. Dark modules SHALL be black and light modules #d0d0d0, drawn one pixel per module and scaled without smoothing to a 200 x 200 px PNG image. QR codes SHALL encode `http://<host name>/configurator` on the profile page, in the web configurator view and on the onboarding Finish step, where the host name is the remote's own, whichever address the text next to it shows. That text SHALL offer the IP address as well, switched by a tap, because a `.local` host name does not resolve in every network (`profiles`, `onboarding`). The onboarding Terms step SHALL encode `https://unfoldedcircle.com/legal`.

#### Scenario: Web configurator QR code
- **WHEN** the remote's host name is `remote-3` and the address text was tapped to show the IP address
- **THEN** the QR code encodes `http://remote-3/configurator`

#### Scenario: Terms QR code
- **WHEN** the onboarding Terms step is shown
- **THEN** a 300 px QR code of `https://unfoldedcircle.com/legal` is displayed

### Requirement: Embedded assets and fonts
The binary SHALL embed the icon font in its Free edition, its name mapping, its fallback mapping, the dock pictures (Dock 2; Dock 3 dark and silver, each charging and not charging), a small loading image, the quarter-arc image of the startup animation and the keypad picture of the button simulator. Text SHALL use the font families "Poppins" (primary, default 30 px) and "Space Mono" (secondary, default 24 px), each also offered with the first letter of every word or all letters capitalised, and the status bar clock SHALL use Poppins 24 px with 110 % letter spacing. These text fonts SHALL NOT be embedded: they are taken from the fonts installed on the system (the device image, or e.g. `~/.fonts` on a desktop), and Qt's default font is used when they are missing. Emoji in any text SHALL be drawn by the Noto Color Emoji font installed on the system, never by the icon font, whose emoji code points are removed (ADR 0010). Custom icons, TV channel icons, background images, sounds, sound effects, legal documents and the licensed icon font SHALL only come from the file system.

#### Scenario: Desktop without Poppins
- **WHEN** the simulator runs on a desktop where Poppins and Space Mono are not installed
- **THEN** all text is drawn in the system's fallback font while icons still render from the embedded icon font

#### Scenario: Emoji in an entity name
- **WHEN** an entity named "Living room 🛋️" is shown on a tile on the device
- **THEN** the emoji is drawn in colour by Noto Color Emoji, not as a monochrome icon glyph

#### Scenario: Startup log
- **WHEN** the icon font loads successfully at startup
- **THEN** "Icon font loaded:", the family the font registered and the path it was loaded from — the file named in `UC_ICON_FONT_PATH` or the embedded resource — are logged

### Requirement: Icon font edition
The icon font SHALL exist in two editions that differ only in which of the mapped icon names they can draw and in the drawing weight: the redistributable **Free** edition, which SHALL be the one kept in the repository and embedded in every binary — every developer build, every unit test, every CI job, every release artefact and every build from the published sources — and the licensed **Pro** edition, which SHALL never be committed to any repository and never be built into a binary; the firmware SHALL install it as a file on the device and name it in `UC_ICON_FONT_PATH`. Both editions and the mapping SHALL come from one pinned Font Awesome release, 6.7.2, which SHALL be the same release the web-configurator uses (ADR 0010). At start-up the UI SHALL load the file named in `UC_ICON_FONT_PATH` when the variable is set and the file exists, is readable and loads as a font, and the embedded Free edition otherwise, logging a warning with the path when a path was set but could not be used; exactly one icon font SHALL be registered. Both editions SHALL carry the same font family name, so that no part of the UI names an edition. About 1400 of the 3891 mapped icon names SHALL resolve in the Free edition and about 3400 in the Pro edition, the rest being brand icons that neither font contains; a mapped name the loaded edition cannot draw is replaced as specified in "Missing and default icons". The repository check SHALL fail if the icon font kept in the repository is not the Free edition, does not match its recorded provenance, or still carries its original vendor family name.

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
- **THEN** the Free font, the mapping and the firmware's Pro file are rebuilt from the new release together, the web-configurator moves to the same release, and the changelog names the icons that were added and the glyphs that were redrawn
