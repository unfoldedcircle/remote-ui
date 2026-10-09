## Context

### Current State Analysis (the design-system stack and its fixes)

- `LicensePage.qml` splits the licenses overview, `<UC_LEGAL_PATH>/licenses/README.md`, at its `## ` headings and
  shows each block as Markdown. The first link the user taps replaces the page with the linked file, split into
  lines, each line rendered as rich text, and every further link is ignored. BACK and the back arrow call the
  settings' `goBack()` and leave for the About page.
- The license files of the core-simulator stand in for the firmware's: the overview links
  `remote-core_licenses.md` (397 kB), `integration-hass_licenses.md` (519 kB) and
  `web-configurator_licenses.md` (9 kB, one table). The folder also holds two `.html` files; nothing links them.
  The crate license files have one `## ` section of up to 517 kB, 691 and 247 headings, a license text in a fenced
  code block under each `#### License`, an overview of anchor links such as `[MIT License](#MIT)` to their `### MIT`
  headings, and 514 and 243 bare web addresses.
- Qt's Markdown importer sizes a heading by its level, from +3 (`#`, about twice the text) to -2 (`######`), and
  takes the colour of a link from the application palette, ignoring `Text.linkColor`; it does not underline
  links. It draws code in the fixed-pitch font of the system at that font's size and turns bare web and mail
  addresses into links (GitHub dialect).

### Constraints

- ADR 0012: the preparation of the text is logic and belongs in C++; the page shows the blocks and navigates.
- ADR 0009: each fix comes with a unit test where one can reproduce it.
- The design system (ADR 0019): the prose role for legal texts, nothing below 22 px, links in `textPrimary`.
- Nothing is fetched from the network; a link only leads to a document of the legal directory (unchanged).

## Goals / Non-Goals

**Goals:**

- A Markdown license reads as formatted text at the type sizes of the design system, and any other file as text.
- BACK leads back the way the user came.
- What looks like a link opens something.

**Non-Goals:**

- A layout of Markdown tables for the narrow screen: a table keeps its columns, its cells wrap.
- The other legal pages (Regulatory, Terms & conditions, Warranty information), which show rich text.

## Decisions

### D1 — The file type decides the format

A link whose file name ends in `.md` is shown as Markdown, any other file as plain text, one block per line. Rich
text was right for no license file: it showed Markdown markup and has nothing to show in plain text.

### D2 — A linked document is split at every heading outside a code block

One `Text` per block keeps the list fast: the largest block of the crate license files is 25 kB instead of
517 kB. The overview keeps its `## ` split. A `#` line inside a code block is no heading.

### D3 — Every heading is a level 4 heading

Level 4 is the size of the text, in bold: close to the section heading role (Poppins 26 Medium). The levels of a
license file carry no meaning a reader on the remote needs.

### D4 — Links: underlined HTML anchors for what opens, text for what does not

A link to another document or to a heading becomes `<a href>`, which Qt underlines, so it stands out in the text
colour. A link with a URL scheme becomes its text with the address in parentheses, and the `://`, `@` and `www.`
of an address are escaped, so the importer does not turn them into links again. A heading link goes to the first
block whose heading reads the anchor or has it as its GitHub-style anchor.

- *Alternative: a palette colour other than `textPrimary` for links.* The design system gives links the text
  colour. Rejected.

### D5 — Code is text

A code block flows into paragraphs, its lines trimmed and every ASCII punctuation mark escaped; inline code the
same. The license texts are prose wrapped at 80 characters; on a 440 px column they read better as paragraphs.

### D6 — The palette's link colour is `textPrimary`

`UiController` sets it once next to the `colors` context property. It applies to every Markdown text: the release
notes and the integration setup texts get the link colour the design system already specifies for them.

### D7 — The way back stays in the page

The page keeps the documents opened by links, each with the block that held its link. It is navigation, which
ADR 0012 leaves to QML.

## Risks / Trade-offs

- [A table of five columns on 440 px has narrow columns, words break inside] → nothing is cut off; a layout for
  tables is an open question.
- [A heading link moves within the document and is not a step of the way back] → BACK leaves the document; the
  overview of the document is one d-pad scroll away.
- [A link label with `<`, `>` or `&`] → stays a Markdown link: Qt inserts an entity inside an HTML anchor out of
  order. None of the license files has one.
- [An indented code block outside a fence] → its escaped addresses show a backslash. None of the license files
  has one.

Resource impact: the blocks are prepared once per document in C++; a 519 kB file gives 247 blocks.

## Migration Plan

- No migration; rollback is a revert.
- Verification target: unit tests, lint, design check, desktop build, and a walk through the Licenses page of the
  desktop simulator with the core-simulator's license files. The device check covers the firmware's files.

## Open Questions

- Should a Markdown table become a list of its rows, each row's first cell as its title?
