# ADR Review Manifest

## ADR Review Completed

- Date: 2026-11-06
- Reviewer: Markus Zehnder (drafted with Claude Code)
- Change: svg-custom-icons

## In-Force ADR Context Reviewed

All ADRs 0001–0020 are accepted and none is superseded. Relevant to this change:

- adr/0002-qt-5-15-lts-pinned.md - the decoder uses `QImageReader::setScaledSize` and the Qt 5.15 SVG image format plugin; no Qt 6 API
- adr/0003-static-aarch64-binary-from-the-public-toolchain.md - the SVG plugin is already linked into the static binary (`QT += svg`); custom icons stay firmware-provided data read from disk
- adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md - the UI does not validate SVG content; that is the core's upload validation. The UI only bounds what a decode can cost, which is a UI resource concern
- adr/0009-unit-tests-for-new-logic-and-bug-fixes.md - the decoder limits and the icon file lookup have their own test targets (`testImageDecoder`, `testIconFiles`)
- adr/0010-icon-font-free-embedded-pro-from-the-firmware.md - an unusable icon file shows the placeholder glyph, which the Free font draws
- adr/0011-qmake-builds-the-app-cmake-builds-the-tests.md - the new unit is registered in `remote-ui.pro` and in every test target that compiles its users
- adr/0012-qml-is-presentation-only-logic-and-models-in-cpp.md - the limits and the file checks are C++; `Icon.qml` only switches to the placeholder on `Image.Error`
- adr/0013-logging-through-qt-categories-to-journald.md - rejected files and failed decodes are logged through `lcResources` and `lcMediaPlayer`, never with the artwork URL
- adr/0014-media-artwork-fetched-by-the-ui-cached-in-memory.md - the artwork is still fetched and cached by the UI; only its decoding is bounded

## Repository-Level ADRs Created

- None: the decisions of this change (one bounded decoder, the file name used as is, the placeholder for an unusable file, the two limits) are tactical to image handling and recorded in `design.md`; they establish no boundary or contract beyond it.

## Notes

The placement of SVG content validation in the core rather than in the UI follows ADR 0005; it is not decided anew here.
