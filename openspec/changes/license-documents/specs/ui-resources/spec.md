## MODIFIED Requirements

### Requirement: Links inside legal documents
Activating a link whose target contains `http` SHALL do nothing. The first other link on a legal page SHALL replace the page content with the linked file, read relative to the document's folder, as rich text; every further link on that page SHALL be ignored until the page is opened again. The Licenses page shows and follows its links as "Linked license documents" describes. A link to a file that cannot be read SHALL leave the page empty. A link SHALL only be followed when it is a relative reference that stays inside the legal directory (`UC_LEGAL_PATH`): a link with any URL scheme (`http`, `https`, `ftp`, `mailto`, `file`, …), a protocol-relative link (`//host/…`), an absolute path and a path that leaves the legal directory through `..` SHALL be refused, logged and treated like a file that cannot be read. Nothing SHALL ever be fetched from the network. The linked file SHALL be found independently of the working directory of the app, and relative references inside it, such as images, SHALL be resolved against the linked file's own folder.

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

## ADDED Requirements

### Requirement: Linked license documents
On the Licenses page a linked file whose name ends in `.md` SHALL be shown as Markdown, split into blocks at every heading outside a code block and inside code into blocks of about 2000 characters, where a paragraph ends, and any other file as plain text, one block per line. A word or table cell wider than the screen SHALL wrap instead of being cut off. A link of a linked document SHALL be followed like a link of the overview.

#### Scenario: Markdown license
- **WHEN** the user taps the link to `web-configurator_licenses.md`
- **THEN** its table is shown as a table, every cell within the width of the screen, and no Markdown markup is visible

#### Scenario: Long license text
- **WHEN** the operating system licenses, 1.8 MB with a license text of 138 kB in one code block, are opened
- **THEN** no block of the page holds more than about 4000 characters

#### Scenario: Text license
- **WHEN** the user taps a link to `qt/LICENSE.txt`
- **THEN** its lines are shown as they are written, as plain text

### Requirement: Back from a linked license
On the Licenses page BACK and the back target of the title bar SHALL reopen the document that held the link of the shown document, at the block of that link, and only on the overview leave for the About page.

#### Scenario: Back to the overview
- **WHEN** the user opens the crate licenses from the overview and presses BACK
- **THEN** the overview is shown at the block with that link
- **AND** BACK once more shows the About page

### Requirement: Headings and code on the Licenses page
In the Markdown of the Licenses page every heading SHALL be bold at the size of the text, whatever its level, and code, inline or as a block, SHALL be text in the prose role: a code block flows into paragraphs, and nothing inside code is read as Markdown. A row of a text table in code SHALL keep its own line, without the borders of the table, and a code line holding only "." SHALL end a paragraph.

#### Scenario: License text in a code block
- **WHEN** a crate license holds its license text in a fenced code block
- **THEN** the text is shown at the size of the text around it, flowing into paragraphs

#### Scenario: Text table in a license text
- **WHEN** the Mesa license in the operating system licenses shows its table of components
- **THEN** every row of the table is a line of its own, and its border lines are not shown

#### Scenario: First-level heading
- **WHEN** a license starts with a `#` heading
- **THEN** the heading is bold at the size of the text

### Requirement: Links on the Licenses page
In the Markdown of the Licenses page only a link the remote can open SHALL be a link, underlined: one to another document of the legal directory or to a heading. A heading link SHALL show the first block with that heading. A link with a URL scheme SHALL be shown as its text followed by its address in parentheses, and an address in the text SHALL be text.

#### Scenario: Web link
- **WHEN** the overview links "SQLite" to `https://www.sqlite.org/`
- **THEN** it shows "SQLite (https://www.sqlite.org/)" as text

#### Scenario: Heading link
- **WHEN** the user taps "MIT License", a link to `#MIT`, in the crate licenses
- **THEN** the page shows the block of the first "MIT" heading
