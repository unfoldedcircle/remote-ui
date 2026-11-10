# ADR 0021 — The web engine comes from a firmware library, loaded at runtime through a versioned C interface

|                |                                                                                                   |
| -------------- | ------------------------------------------------------------------------------------------------- |
| **Status**     | Proposed                                                                                          |
| **Supersedes** | — (none)                                                                                          |
| **Date**       | 2026-11-10                                                                                        |
| **Deciders**   | Markus Zehnder                                                                                    |
| **Related**    | [0002](0002-qt-5-15-lts-pinned.md), [0003](0003-static-aarch64-binary-from-the-public-toolchain.md), [0004](0004-gpl-3-license-and-published-source.md), [0016](0016-the-ui-touches-as-little-hardware-as-possible.md), `openspec/specs/platform-constraints`, OpenSpec change `web-view` |

## Context

The UI is to show web pages made for the remote's screen and input. A web engine is a large native
code base with dependencies of its own (a network stack, ICU, media and graphics libraries) and helper
processes for the web content and the network.

- The UI is one static binary built with the public toolchain image (ADR 0003). That image has no web
  engine: Qt WebEngine cannot be linked statically, and Qt WebView uses it on Linux. WPE WebKit and its
  dependencies would have to enter the toolchain image, the unstripped WebKit library alone (about
  160 MB) is larger than the binary's 100 MB budget of `platform-constraints`, and WebKit's helper
  processes would still have to come from the firmware.
- The firmware (Unfolded OS) builds its system libraries with Buildroot and can build WPE WebKit like
  any other. It ships it behind a small library of its own, `libucwebview.so.1`, whose public header
  (MIT) uses no GLib, WPE or EGL types: a plain C interface with an API version, functions that name
  the library release that added them, and a callback struct the host passes with its size.
- The binary already uses system code of the firmware: it links GLib, EGL, GLESv2, fontconfig and
  other system libraries, and Qt loads OpenSSL at runtime by name. ADR 0003's rule that "no
  application code or QML is loaded from disk at runtime" is about the UI's own code; it does not
  cover a system library of the firmware. ADR 0003 also expects a new system library to come through
  the toolchain image. This library does not, because the UI does not link it.
- A custom UI build and the firmware are updated independently (ADR 0004): a user installs a newer
  or older custom UI on any firmware, and the firmware updates without the custom UI.

## Decision

- **The web engine is WPE WebKit, provided by the firmware behind `libucwebview.so.1`.** The UI never
  links it and no web engine enters the toolchain image or the binary.
- **The UI loads the library at runtime, on first use**, never on the start-up path, resolves its
  functions once and uses it only when the library's API version equals the one the UI was built
  with. The library's public header is copied verbatim into the repository, apart from one comment
  line, and is the only contract between the two.
- **Within one API version every library release works.** Functions a later release added are
  optional and their absence is logged once; the features that need them are missing, nothing else.
  An incompatible change of the library raises its API version, and a UI built for the old one
  reports the web view unavailable.
- **The web view is optional everywhere.** On a firmware without the library, with another API
  version, in the desktop simulator and in a build with `CONFIG+=no-webview` the UI runs as without
  the web view, and screens that use it hide their entry.
- **The library's threading contract is the UI's.** Every call is made on the GUI thread, and WebKit's
  events run through GLib's default main context on Qt's GLib event dispatcher.
- **This is the way for a native engine the toolchain cannot carry,** not a way around the toolchain
  for ordinary dependencies: those stay statically linked (ADR 0003). Another library loaded this way
  needs its own decision.

## Consequences

- **Easier:** web pages without a toolchain change and with a binary that grows by less than 0.5 MB;
  WebKit is updated with the firmware, with its security fixes; older firmware and the simulator are
  unaffected; the custom UI and the firmware keep their independent updates.
- **Harder / accepted:** what the web view can do depends on the firmware's library release; an API
  version change of the library needs a new UI build; the web view cannot be checked in the desktop
  simulator, only on a remote with the library (ADR 0016, `desktop-simulator`); WebKit's UI-process
  side runs inside the UI process, so a crash there ends the UI, and the page's own processes run in
  the UI's service, so their memory is the UI's to account for (`platform-constraints`); the item uses
  Qt's private native interface for the EGLDisplay (`gui-private`), which ADR 0002's pin of Qt 5.15
  keeps stable; the web view needs Qt's GLib event dispatcher, so `QT_NO_GLIB=1` turns it off.
- **Licences:** WPE WebKit (LGPL-2.1 and BSD) and its dependencies are firmware components with the
  firmware's licence information; the repository holds only the library's MIT header (ADR 0004).
