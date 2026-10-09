One phase per fix, then the verification. Each fix commit carries its `CHANGELOG.md` entry.

## 1. A linked license by its file type

- [x] 1.1 `Resources::isMarkdownFile()` and `Resources::licenseBlocks()`: Markdown split at the overview's `## `
      headings or at every heading of a linked document outside code blocks, every heading at level 4; any other
      text one block per line. The page's QML splitting removed.
- [x] 1.2 `LicensePage.qml`: Markdown or plain text by the file type, `Text.Wrap`.
- [x] 1.3 Tests in `test/ui/test_resources.cpp`: `isMarkdownFile`, plain text, overview split, linked document
      split outside code, headings at the text size.

## 2. Back from a linked license

- [x] 2.1 `LicensePage.qml`: the documents opened by links with the block of their link; BACK and the back target
      of the title bar reopen the previous one, and leave for the About page on the overview.

## 3. Markdown links in the text colour

- [x] 3.1 `UiController`: the application palette's link colour is `textPrimary`.

## 4. Links the remote can open

- [x] 4.1 `Resources::licenseBlocks()`: links to documents and headings as HTML anchors, web and mail links as
      text with their address, autolinks and bare addresses as text; `Resources::licenseAnchorBlock()`.
- [x] 4.2 `LicensePage.qml`: a heading link shows its block.
- [x] 4.3 Tests: web and mail links, autolinks, bare addresses, an odd backtick, anchors, heading lookup.
- [x] 4.4 The d-pad walks the links: `Resources::licenseLinks()` and `Resources::licenseBlockWithSelection()`
      with tests; `LicensePage.qml` selects with DPAD_DOWN / DPAD_UP, opens with OK and selects the link again
      on the way back. A label with a plain "&" is an anchor as well.

## 5. Code as text

- [x] 5.1 `Resources::licenseBlocks()`: code blocks and inline code as escaped text, code blocks flowing into
      paragraphs.
- [x] 5.2 Test: a code block and inline code.
- [x] 5.3 A text table in a code block keeps one line per row, without its borders: the Mesa license in the
      operating system licenses of the firmware. Test with a table of two rows.
- [x] 5.4 Code in blocks of about 2000 characters, split where a paragraph ends (also at a Debian "." line) or
      at a line in a longer paragraph, after the measurement on a Remote 3 (6.5). Tests for both splits.

## 6. Verification

- [x] 6.1 `make test`: all targets pass.
- [x] 6.2 `./cpplint.sh` clean, `./design-check.sh` 0 problems, `qmllint` on the changed QML file.
- [x] 6.3 `make linux`; a walk through the Licenses page of the desktop simulator at 480 x 800 with the
      core-simulator's license files: overview, crate licenses, a heading link, code as text, BACK and the back
      arrow; no QML error or binding loop in the log.
- [x] 6.4 `openspec validate license-documents --strict`.
- [x] 6.5 On a Remote 3 with the firmware's license files, measured with a custom build: the time to prepare
      the blocks and the longest frame while the largest blocks come into view. First run: 2.8 s for the 138 kB
      block, which 5.4 splits; with 5.4 no step takes more than 387 ms (opening the operating system licenses), a
      jump to a large block at most 198 ms.
- [ ] 6.6 On both remotes: the license overview and a license read through with the d-pad and by touch.
