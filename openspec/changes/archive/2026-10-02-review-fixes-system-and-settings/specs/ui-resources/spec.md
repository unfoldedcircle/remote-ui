## MODIFIED Requirements

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
