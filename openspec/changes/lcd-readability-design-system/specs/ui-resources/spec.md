## MODIFIED Requirements

### Requirement: Legal documents
The About entries Regulatory, Terms & conditions and Warranty information SHALL show the alphabetically first file (ignoring case) ending in `.html` or `.md` in `<UC_LEGAL_PATH>/regulatory`, `<UC_LEGAL_PATH>/terms` and `<UC_LEGAL_PATH>/warranty` respectively, rendered as rich text regardless of the extension, word-wrapped in the prose role within the 20 px gutter, with the document's folder as base for relative references such as images. The Regulatory entry SHALL be offered on every model. The onboarding Terms step SHALL show the same Terms document in its popup. A missing folder or file SHALL show an empty page. DPAD_DOWN / DPAD_UP SHALL scroll by half the height of the text per press with a 300 ms animation, clamped to the content, repeating while the key is held.

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
The Licenses entry SHALL show `<UC_LEGAL_PATH>/licenses/README.md` as Markdown, split into sections at every line starting with `## `, each section rendered as its own block of a scrolling list in the prose role within the 20 px gutter. DPAD_DOWN / DPAD_UP SHALL scroll it by half its height per press as on the other legal pages.

#### Scenario: License overview
- **WHEN** the README contains an introduction and three `## ` headings
- **THEN** the page shows four blocks, the headings formatted as Markdown headings
