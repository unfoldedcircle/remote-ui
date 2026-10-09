## Why

The Licenses page of the About settings showed a license opened from its overview line by line as rich text, so a
Markdown license showed its markup, and BACK on it left the licenses for the About page. In the Markdown it does
format, headings reached twice the size of the text, links were Qt's dark blue, web and mail addresses were links
that did nothing, and code, which holds every license text of the crate license files, was drawn in the small
fixed-pitch font of the system.

## What Changes

- A linked file ending in `.md` is shown as Markdown, split into blocks at every heading outside a code block; any
  other file as plain text, one block per line. A word or table cell wider than the screen wraps.
- Every heading is bold at the size of the text.
- BACK and the back arrow reopen the document that held the link, at the block of the link; a link inside a
  linked document is followed as well.
- Markdown links take the primary text colour.
- Only a link the remote can open stays a link, underlined: one to another document of the legal directory or to
  a heading. Web and mail addresses are text, the address after the link text.
- Code is text: a code block flows into paragraphs of the prose role.
- `Resources` prepares the blocks instead of the page's QML (ADR 0012), with unit tests (ADR 0009).

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `ui-resources`: "Links inside legal documents" leaves the Licenses page to four new requirements: "Linked
  license documents", "Back from a linked license", "Headings and code on the Licenses page" and "Links on the
  Licenses page".

## Impact

- Hardware models: Remote Two and Remote 3 alike.
- Core-API: none.
- Code: `src/ui/resources.h`, `src/ui/resources.cpp`, `src/ui/uiController.cpp` (the palette's link colour),
  `src/qml/settings/about/LicensePage.qml`; tests in `test/ui/test_resources.cpp`, no change to the test target.
- Translations: none.
- Stacked on the design-system pull requests and the fixes on top of them.
- Third-party code and assets: none.
- Docs: `CHANGELOG.md`.
