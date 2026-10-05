## ADDED Requirements

### Requirement: Icon identifiers
An icon SHALL be referenced by a string `<prefix>:<name>`, where only the text between the first and the second colon is used as the name. A `uc:` icon (any prefix containing `uc`) SHALL resolve to a glyph of the embedded icon font through the embedded name mapping of 3877 names (Font Awesome 6 Pro glyphs plus Unfolded Circle additions such as `remote`, `blind` or `activity`). A `custom:` icon (any prefix containing `custom`) SHALL resolve to the file `<UC_RESOURCE_PATH>/Icon/<name>`. An identifier containing `ctv:` SHALL resolve to the TV channel icon file `<UC_RESOURCE_PATH>/TvChannelIcon/<name>`. A trailing `.png`, `.jpg` or `.jpeg` SHALL be kept as part of the file name.

#### Scenario: Built-in icon
- **WHEN** a tile shows the icon `uc:lightbulb`
- **THEN** the lightbulb glyph of the icon font is drawn

#### Scenario: Custom icon
- **WHEN** a tile shows the icon `custom:sofa.png` and `<UC_RESOURCE_PATH>/Icon/sofa.png` exists
- **THEN** that image file is drawn

#### Scenario: TV channel icon
- **WHEN** a tile shows the icon `ctv:bbc1.png` and `<UC_RESOURCE_PATH>/TvChannelIcon/bbc1.png` exists
- **THEN** that image file is drawn

### Requirement: Missing and default icons
An icon that cannot be resolved SHALL leave its area empty and SHALL only be logged: an empty identifier, an identifier without a colon, an unknown prefix, or a custom or TV channel icon whose file does not exist. A `uc:` name that is not in the mapping SHALL fall back to a TV channel icon file of the same name and otherwise stay empty. An entity without an icon SHALL get a default icon by type: button and switch `uc:power-on`, climate `uc:climate`, cover `uc:blind`, light `uc:light`, media player `uc:music`, sensor `uc:sensor`, remote `uc:remote`, activity and macro `uc:activity`, voice assistant `uc:microphone`, select `uc:list-dropdown`, any other type `uc:warning`.

#### Scenario: Custom icon file deleted
- **WHEN** an entity uses `custom:old.png` and the file no longer exists
- **THEN** the tile shows no icon and no placeholder

#### Scenario: Unknown built-in name
- **WHEN** an entity uses `uc:does-not-exist`
- **THEN** the icon is empty unless `<UC_RESOURCE_PATH>/TvChannelIcon/does-not-exist` exists, in which case that file is drawn

#### Scenario: Entity without icon
- **WHEN** the core delivers a light entity without an icon
- **THEN** the light shows `uc:light`

### Requirement: Icon rendering
A glyph icon SHALL be drawn centred in its square box in the icon font at a pixel size of half the box size (rounded), off-white unless a colour is given, with native text rendering. An image icon SHALL be decoded asynchronously at the box size, scaled to fit the box with its aspect ratio preserved, and kept in the image cache. If the embedded icon font cannot be loaded at startup the UI SHALL log "Icons failed to load" and continue.

#### Scenario: Glyph size
- **WHEN** an icon is shown in an 80 px box
- **THEN** the glyph is drawn at 40 px

#### Scenario: Non-square custom image
- **WHEN** a 200 x 100 custom icon is shown in an 80 px box
- **THEN** it is drawn 80 x 40, centred

### Requirement: Icon selector
The icon selector SHALL be a full-screen overlay titled "Select icon" with two tabs, "Unfolded Icons" and "Custom Icons", a grid of 120 x 120 px cells each showing a 100 px icon, and a "Close" button. The Unfolded tab SHALL list every name of the icon font mapping as `uc:<name>`, sorted by name; the Custom tab SHALL list every `.png`, `.jpg` and `.jpeg` file (extension matched case-insensitively) in `<UC_RESOURCE_PATH>/Icon/` as `custom:<file name>`, sorted by name ignoring case. Both lists SHALL be read when the selector is created, not each time it opens. There SHALL be no search field and no swiping between tabs. Tapping an icon SHALL select it and close the overlay; the selected identifier is applied to the entity (from the page edit menu) or the profile (from the profile menu). Opening SHALL start on the Unfolded tab with its first icon.

#### Scenario: Selecting an icon
- **WHEN** the user taps the `uc:tv` cell
- **THEN** the overlay closes and `uc:tv` becomes the icon of the entity or profile being edited

#### Scenario: No resource directory
- **WHEN** `UC_RESOURCE_PATH` is unset or its `Icon` folder is empty
- **THEN** the Custom Icons tab shows an empty grid

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
Activating a link whose target contains `http` SHALL do nothing. The first other link on a legal page SHALL replace the page content with the linked file, read relative to the document's folder, as rich text; on the Licenses page each line of the linked file becomes its own block. Every further link on that page SHALL be ignored until the page is opened again. A link to a file that cannot be read SHALL leave the page empty.

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

### Requirement: QR codes
The UI SHALL generate QR codes itself from the UTF-8 text, with error correction level Low (raised to a higher level when the text still fits the same symbol version), the smallest symbol version that fits, an automatically chosen mask and no quiet zone. Dark modules SHALL be black and light modules #d0d0d0, drawn one pixel per module and scaled without smoothing to a 200 x 200 px PNG image. QR codes SHALL encode `http://<web configurator address>/configurator` on the profile page, in the web configurator view and on the onboarding Finish step, and `https://unfoldedcircle.com/legal` on the onboarding Terms step.

#### Scenario: Web configurator QR code
- **WHEN** the web configurator address is `192.168.1.20`
- **THEN** the QR code encodes `http://192.168.1.20/configurator`

#### Scenario: Terms QR code
- **WHEN** the onboarding Terms step is shown
- **THEN** a 300 px QR code of `https://unfoldedcircle.com/legal` is displayed

### Requirement: Embedded assets and fonts
The binary SHALL embed the icon font, its name mapping, the dock pictures (Dock 2; Dock 3 dark and silver, each charging and not charging), a small loading image, the quarter-arc image of the startup animation and the keypad picture of the button simulator. Text SHALL use the font families "Poppins" (primary, default 30 px) and "Space Mono" (secondary, default 24 px), each also offered with the first letter of every word or all letters capitalised, and the status bar clock SHALL use Poppins 24 px with 110 % letter spacing. These text fonts SHALL NOT be embedded: they are taken from the fonts installed on the system (the device image, or e.g. `~/.fonts` on a desktop), and Qt's default font is used when they are missing. Custom icons, TV channel icons, background images, sounds, sound effects and legal documents SHALL only come from the file system.

#### Scenario: Desktop without Poppins
- **WHEN** the simulator runs on a desktop where Poppins and Space Mono are not installed
- **THEN** all text is drawn in the system's fallback font while icons still render from the embedded icon font

#### Scenario: Startup log
- **WHEN** the icon font loads successfully at startup
- **THEN** "Icons loaded" is logged
