# ADR Review Manifest

## ADR Review Completed

- Date: 2026-09-19
- Reviewer: Markus Zehnder
- Change: add-missing-keyboard-layouts

## In-Force ADR Context Reviewed

- docs/adr/0002-qt-5-15-lts-pinned.md - the layouts use Qt 5.15 Virtual Keyboard QML only.
- docs/adr/0003-static-aarch64-binary-from-the-public-toolchain.md - a layout exists at runtime only
  when it is registered in `resources/qrc/keyboard.qrc`.
- docs/adr/0006-en-us-ts-is-the-only-edited-translation.md - unaffected: keyboard layouts are code,
  not translations, and a new interface language now needs both.
- docs/adr/0008-remote-two-and-remote-3-with-feature-parity.md - the keyboard is identical on both
  remotes.

## Repository-Level ADRs Created

- None: no major durable architectural decisions were introduced by this change. That the keyboard
  language follows the interface language, with no language-switch key, is existing behaviour
  recorded in the `on-screen-keyboard` capability.

## Notes

The layout style decision (no Enter key, no language switch, no handwriting, a hide-keyboard key,
shared symbols pages) is a house style captured in the `on-screen-keyboard` spec rather than an
ADR; it applies to every layout and is enforced by review.
