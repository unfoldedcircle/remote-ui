# ADR Review Manifest

## ADR Review Completed

- Date: 2026-11-08
- Reviewer: Markus Zehnder (drafted with Claude Code)
- Change: wide-icons

## In-Force ADR Context Reviewed

All ADRs 0001–0020 are accepted, and none is superseded. These are relevant to this change:

- adr/0002-qt-5-15-lts-pinned.md - The change works around the Qt 5.15 `loadSize` behaviour (fit decodes to the cover size) with Qt 5.15 API only (`QImageReader::size`, `sourceSize`). It uses no Qt 6 API.
- adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md - The core validates SVG uploads and their aspect ratio. The UI does not clamp or reject ratios, also not a width/height that disagrees with the viewBox (a follow-up for the core's upload validation); it lays out what Qt draws.
- adr/0007-one-input-idiom-per-screen.md - The page buttons are not reached by the d-pad. Their input idiom is unchanged.
- adr/0008-remote-two-and-remote-3-with-feature-parity.md - The same behaviour applies on both models. Only the cell size differs, through the screen height.
- adr/0009-unit-tests-for-new-logic-and-bug-fixes.md - The size lookup, its cache and the fit computation get unit tests in `testIconFiles` and `testImageDecoder`.
- adr/0012-qml-is-presentation-only-logic-and-models-in-cpp.md - The aspect lookup and the fit computation are C++. `Icon.qml` only binds the computed size.
- adr/0018-the-ui-tolerates-core-api-values-it-does-not-know.md - An SVG whose size cannot be read, or whose ratio is outside the core's range, still renders (falling back to today's box request) rather than failing.
- adr/0019-the-design-system-owns-colours-type-and-selection.md - The layout of the page buttons is unchanged and no padding is added, so no new design-system token is needed.

## Repository-Level ADRs Created

- None: no major durable architectural decisions were introduced by this change. The size lookup and the fit raster are tactical details of image handling, recorded in `design.md`.

## Notes

The change builds on `svg-custom-icons`, which is archived. It only adds requirements to
`ui-resources`, so it does not modify the requirements that change introduced.
