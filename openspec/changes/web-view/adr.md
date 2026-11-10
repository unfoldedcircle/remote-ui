# ADR Review Manifest

## ADR Review Completed

- Date: 2026-11-10
- Reviewer: Markus Zehnder (pending: drafted with Claude Code, not yet reviewed)
- Change: web-view

## In-Force ADR Context Reviewed

All ADRs 0001–0020 are accepted, and none is superseded. These are relevant to this change:

- adr/0002-qt-5-15-lts-pinned.md - Qt 5.15 API only. The item uses Qt's private native interface for the EGLDisplay (`QT += gui-private`); the pin keeps that private API stable.
- adr/0003-static-aarch64-binary-from-the-public-toolchain.md - The binary stays static and is built with the same toolchain image; no web engine enters either. ADR 0003's "no application code or QML is loaded from disk" is read as the UI's own code: the web engine is a system library of the firmware, as GLib, EGL and OpenSSL already are. ADR 0021 records the decision; see Notes.
- adr/0004-gpl-3-license-and-published-source.md - The library's public header (MIT) is copied into the repository and needs the lead developer's approval. WPE WebKit (LGPL-2.1 and BSD) stays in the firmware. Whether the header needs an entry in the licences of the About screen is an Open Question of design.md.
- adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md - The web view sends no Core-API message and holds no state of the core.
- adr/0007-one-input-idiom-per-screen.md - The developer page walks its controls with a focus chain; the web view takes the d-pad itself and leaves the remote's own buttons to the page's `ButtonNavigation` (path 1), so every key keeps one idiom. `docs/key-navigation.md` gains the pattern.
- adr/0008-remote-two-and-remote-3-with-feature-parity.md - The same feature on both remotes; pinch zoom needs a touch panel that reports two fingers, which only the Remote Two has.
- adr/0009-unit-tests-for-new-logic-and-bug-fixes.md - Six web view test targets run the item against a stubbed library; the `InputField` fix is covered by the hide-key test of `testWebViewKeyboard`, and the version header by the build.
- adr/0012-qml-is-presentation-only-logic-and-models-in-cpp.md - Input mapping, URL rules, the frame queue, the pinch transform and the request objects are C++ with unit tests. The developer page still decides a few things in QML (address completion, PDF types, the request queue, notification texts): an Open Question of design.md.
- adr/0013-logging-through-qt-categories-to-journald.md - A new category `uc.ui.webview`; secrets never reach a line. The form of the redacted addresses (`?...` instead of `<redacted>`) and the trace switches are an Open Question of design.md.
- adr/0016-the-ui-touches-as-little-hardware-as-possible.md - The web view touches no hardware of its own; the frames go through Qt's EGL display. The simulator has no web view, which the `desktop-simulator` delta names.
- adr/0019-the-design-system-owns-colours-type-and-selection.md - The developer page and the dialogs use colour tokens and type roles only; the design check passes on every commit.
- adr/0020-two-selection-styles-fill-in-menu-lists-ring-on-setting-screens.md - The web view shows the ring of a setting screen while the d-pad is in it.

## Repository-Level ADRs Created

- docs/adr/0021-web-engine-from-a-firmware-library-loaded-at-runtime.md - The web engine is WPE WebKit from the firmware behind `libucwebview.so.1`, loaded at runtime on first use through its versioned plain C interface, never linked; every library release of the API version works, and the UI runs unchanged without it.

## Notes

ADR 0021 does not supersede ADR 0003. ADR 0003 keeps everything it decided: one static binary from
the public toolchain image, everything of the UI's own compiled in, the sandbox. ADR 0021 adds a kind
of dependency ADR 0003 did not foresee, a firmware library behind a C interface that the UI loads at
runtime instead of linking. If the maintainer reads ADR 0003's "no application code is loaded from
disk at runtime" as covering such a library, ADR 0021 has to supersede ADR 0003 instead and restate
its other decisions.
