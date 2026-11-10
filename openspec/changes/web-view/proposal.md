## Why

The UI cannot show web content. A page made for the remote's screen and input, such as a device's own
setup page or a status page, could only be shown by rewriting it in QML. A web engine cannot simply be
built into the UI: the static binary is built with the public toolchain image, which has no web engine
(Qt WebEngine is not available, ADR 0003), and the binary has to stay below 100 MB
(`platform-constraints`).

The firmware (Unfolded OS) now ships WPE WebKit behind a small library with a versioned plain C
interface, `libucwebview.so.1`. The UI can load it at runtime, without a toolchain change and without
growing the binary noticeably. The web view is implemented on two branches; the workflow asks for the
plan to be written down before such work is reviewed, so this change records it.

## What Changes

- A `WebView` QML item (`UC.WebView 1.0`) shows a web page rendered by the firmware's WPE WebKit. It
  loads the firmware's library at runtime and uses it only with the API version it was built for.
  Without the library it is an empty placeholder that reports itself unavailable, and nothing else in
  the UI changes. `CONFIG+=no-webview` builds the app without the web view.
- A developer page, Settings > Web view, listed only when the library loads: an address field with
  Load, Back, Forward and Stop/Reload, a status line and the page. Touch, the d-pad and the on-screen
  keyboard work in the page. PREV and NEXT go back and forward, VOLUME zooms the page, CHANNEL scrolls
  it a screen, and two fingers pinch-zoom it on a panel that reports two fingers (Remote Two).
- The page's `alert()`, `confirm()`, `prompt()`, its leave-page question and HTTP logins open as
  dialogs on the developer page. Downloads, PDF documents and refused new windows show a notification
  instead of a failed load, and `window.close()` goes back or leaves the page.
- A trace for device tests (`UC_WEBVIEW_TRACE=1`) that logs no typed text, and addresses in every log
  line without user name, password, query and fragment.
- Shared components: a text field whose keyboard follows the keypad focus keeps the focus when the
  keyboard goes down. This fixes the hide key on the Wi-Fi password and Wi-Fi setup forms, where the
  keyboard came straight back. `InputField` gets `navLeft` and `navRight`, and `Components.Button` a
  read-only `pressed` for icons on a button.
- Independent of the web view, carried by phase 1: the version string comes from a generated header,
  so a rebuild on a new commit no longer keeps the version of the previous build.
- A new ADR 0021 records the decision to take the web engine from a firmware library loaded at
  runtime.

## Non-Goals

- A browser for any web site: no tabs, bookmarks, downloads, PDF viewer, saved passwords or website
  data kept across restarts (an opt-in data directory exists for tests). The web view is meant for
  pages made for the remote's screen and input; heavy sites are slow and use much memory (design.md,
  "Known limitations").
- A web view on any screen other than the developer page. A user-facing use is a later change.
- Changes to the firmware library. The UI uses its public header only; what the UI needs from future
  library releases is listed under "Follow-ups outside this change".
- An Enter key on the on-screen keyboard (design.md, "Known limitations").

## Capabilities

### New Capabilities

- `web-view`: the web view and its developer page: availability with the firmware's library, the
  page's touch, keypad and on-screen keyboard input, history, zoom, scroll and pinch, the frame rate,
  crash recovery, the page's dialogs and logins, refused content, and what the web view logs.

### Modified Capabilities

- `platform-constraints`: two new requirements: the web engine is a firmware library loaded at runtime
  (ADR 0021), and the web view costs nothing before it is used.
- `on-screen-keyboard`: a field whose keyboard follows the keypad focus keeps the focus when the
  keyboard goes down ("Hiding the keyboard" modified, one requirement added).
- `settings-menu`: the "Web view" entry of the Settings submenu.
- `desktop-simulator`: the simulator has no web view; ADR 0016 asks for every difference between the
  simulator and a remote to be named in this spec.

## Impact

- **Hardware models:** both. Pinch zoom needs a touch panel that reports two fingers: the Remote Two
  has one, the Remote 3's touch driver reports one finger, so the Remote 3 zooms with VOLUME only
  (ADR 0008, hardware only one model has).
- **remote-core / Core-API:** none. The web view sends no Core-API message and needs no new field.
- **Firmware:** the firmware's libucwebview library (Unfolded OS), `libucwebview.so.1`, API version 1.
  Features follow the library release and are optional: 0.2.0 the display's refresh rate, 0.3.0 the
  drag threshold, 0.4.0 the sized callback struct, 0.5.0 the zoom render wait, 0.6.0 pinch zoom, 0.7.0
  dialogs and logins; text links need the font configuration of 0.6.3 or 0.7.1. A custom UI and the
  firmware update independently, so the UI works with every library release of API version 1.
- **Third-party code and licences:** `src/webview/uc_webview.h` is a verbatim copy of the library's
  public header (MIT, Unfolded Circle) with one comment line on top; `src/webview/CPPLINT.cfg` keeps it
  out of the lint. WPE WebKit (LGPL-2.1 and BSD) and its dependencies are part of the firmware, not of
  this repository or the binary. The lead developer approves the header copy (ADR 0004).
- **Qt:** `QT += gui-private` for Qt's EGLDisplay through the platform native interface; Qt 5.15 API
  only (ADR 0002). No new Qt module and no toolchain image change.
- **Build and project files** (AGENTS.md asks before such changes): `remote-ui.pro` gets an optional
  web view block and `version.h.in`; `test/CMakeLists.txt` gets `test/webview/` and a generated
  `version.h`; `resources/qrc/main.qrc` the three new QML files.
- **Code:** new `src/webview/` (the item, the library loader, input mapping, frame queue, texture,
  pinch transform, request objects), `src/qml/settings/settings/WebView.qml`,
  `src/qml/components/WebViewDialog.qml` and `WebViewLogin.qml`; changed `Settings.qml`,
  `InputField.qml`, `Button.qml`, `uiController.{h,cpp}`, `logging.{h,cpp}`, `softwareUpdate.h`,
  `translation.cpp`, `.gitignore`, `en_US.ts`, `CHANGELOG.md`, `docs/key-navigation.md`,
  `docs/README.md`, and a new `docs/webview.md`.
- **Delivery:** two pull requests, one per phase of `tasks.md`: `feat/webview` (the item and its
  developer page) and `feat/webview-dialogs` (dialogs and logins), which builds on the first.

## Follow-ups outside this change

- **Firmware library:** a smaller WebKit cache model, against the memory growth of a view that visits
  many different sites (planned); reporting same-document navigations, so the address field follows a
  single-page application.
- **UI, later changes:** a web view on a user-facing screen, once the open questions on memory
  (design.md) are decided; an Enter key on the on-screen keyboard if web forms need it.
