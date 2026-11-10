# Tasks

The decisions are D1–D21 in design.md. Phase 1 is the pull request of the branch `feat/webview` (the
web view and its developer page), phase 2 the one of `feat/webview-dialogs` (the page's dialogs and
logins). The work was done before this change was written: a ticked task is implemented and verified
on its branch as stated, and lands with that branch's pull request. The open tasks are what is still
missing.

The phases are not file-disjoint. Phase 2 changes files of phase 1 (`uc_webview.h`,
`webviewitem.{h,cpp}`, `webviewlibrary.{h,cpp}`, `WebView.qml`, the web view tests) and is based on
it; its pull request targets `feat/webview` until phase 1 is merged.

## 1. Phase 1: the web view and its developer page (`feat/webview`)

- [x] 1.1 Generate the version into `version.h` from `version.h.in` (`QMAKE_SUBSTITUTES` in
      `remote-ui.pro`) instead of a `-D` define, include it where `APP_VERSION` is used, give the unit
      tests a fixed one from `test/CMakeLists.txt`, and ignore the generated files in `.gitignore`.
      Verified: a desktop rebuild in the build directory of `388c07b`, without `make clean`, reports
      the branch's version (`0.85.0-14-g47d76f5`); `make test` passes.
- [x] 1.2 Copy the library's public header to `src/webview/uc_webview.h` (one comment line on top)
      with `src/webview/CPPLINT.cfg`; add `WebViewLibrary`, which loads `libucwebview.so.1` on first
      use, resolves every function into a table checked against the header with `static_assert`,
      requires the same API version and Qt's GLib dispatcher, and resolves the functions of later
      library releases as optional (D1, D3, D4); add the logging category `uc.ui.webview`. Verified
      with `testWebViewItem` against a stubbed function table, with and without the optional
      functions, and a clean `./cpplint.sh`.
- [x] 1.3 Register the new files: the optional `!no-webview` block in `remote-ui.pro` (sources,
      `UC_WEBVIEW`, `QT += gui-private`), the QML type `UC.WebView 1.0` and `ui.webViewAvailable` in
      the UI controller, and `test/webview/CMakeLists.txt` with its five targets in
      `test/CMakeLists.txt` (D5). Verified: `make linux` and `make ucr2` build; `make linux` with
      `QMAKE_ARGS="CONFIG+=release CONFIG+=no-webview"` builds without any web view object.
- [x] 1.4 Implement the `WebView` item: view lifecycle, size and scale, mapping with the item's and
      the window's visibility, frames through `FrameQueue` and the texture node, release after the
      page flip, load events and history, reload once after a crash, the scheme allowlist, the white
      default background, `runJavaScript()` and `loadHtml()` (D2, D14). Verified with
      `testWebViewItem` (lifecycle, load events, crash recovery, frame release, refused addresses)
      and `testWebViewFrameQueue`.
- [x] 1.5 Forward the input: keys as keysyms with the remote's own buttons left to the UI, touch
      points with slot numbers and the library's drag threshold, the input method contract, the
      keyboard following the focus owner, the tap that brings the keyboard back, and the fields the
      page focused by itself (D6, D8, D9). Verified with `testWebViewInput` (key table, input hints,
      tap distance) and `testWebViewItem` (input method events, taps, drag threshold).
- [x] 1.6 Pass the screen's refresh rate before the first view and on every change (D13). Verified
      with `testWebViewItem` (rate before the view, on a change, a library without the function).
- [x] 1.7 Add the developer page `src/qml/settings/settings/WebView.qml` to `resources/qrc/main.qrc`
      and its entry to the Settings submenu when `ui.webViewAvailable` is true: address field, Load,
      Back, Forward, Stop/Reload, status line, the d-pad map and the focus hand-over from a button
      that turns disabled; its strings with translator comments in `en_US.ts`. Verified with
      `testWebViewKeyboard` (the address field's keyboard, the d-pad map, the history buttons,
      Stop/Reload, address completion, the field following the page's address).
- [x] 1.8 Map PREV/NEXT to the history, VOLUME to the zoom with its render wait, label and spinner,
      and CHANNEL to page up and down, all ignored while the keyboard is up; add pinch zoom with
      `PinchZoom` and the held frame queue (D7, D11, D12). Verified with `testWebViewKeyboard` (from
      every control and from the web view, held keys, ignored while typing, the label and spinner,
      three quick presses with two zoom calls), `testWebViewItem` (zoom levels, render wait, page
      keys) and `testWebViewPinch`.
- [x] 1.9 Keep typed text and address secrets out of the log: the trace rules of
      `UC_WEBVIEW_TRACE`, the redacted addresses and `UC_WEBVIEW_LOG_FULL_URLS` (D15). Verified with
      the password session of `testWebViewItem`, which captures every log message, and the address
      rules of `testWebViewInput`.
- [x] 1.10 Shared components: `InputField` keeps the focus of a field whose keyboard follows the
      keypad focus when the keyboard goes down, and gets `navLeft` and `navRight`; `Components.Button`
      gets a read-only `pressed` (D10, D16). Verified with
      `testWebViewKeyboard::hideKeyClosesTheKeyboardAndItStaysClosed`, which fails without the
      `InputField` change, and the other keyboard cases.
- [x] 1.11 Put the developer page on the design system: tokens, type roles, the 20 px gutter,
      `Components.Button`, the ring around the web view (D16). Verified: `./design-check.sh` reports
      no problem on any commit, and `testWebViewKeyboard` runs without QML warnings.
- [x] 1.12 Document the web view in `docs/webview.md`, the pattern of an item that takes the d-pad and
      `navLeft`/`navRight` in `docs/key-navigation.md`, and the index entry in `docs/README.md`.
      Verified with the docs link check (lychee, offline, with fragments): no error.
- [x] 1.13 Add the `CHANGELOG.md` entries under Unreleased in the commits that make the changes:
      "Added" for the web view steps, "Fixed" for the version string and the keyboard's hide key on
      the Wi-Fi forms. Verified by reading the entries of each commit.
- [x] 1.14 Run the gates on every commit of the branch: `make test` (41 of 41 at the tip, 36 on
      `main`), `make linux`, `./cpplint.sh` and `./design-check.sh`; at the tip also the icon font
      checks and `make ucr2` (48.2 MB, unstripped).
- [ ] 1.15 Check the branch as rebased onto `main` on a Remote Two and a Remote 3 with a firmware
      that ships the library: the device checks of `docs/webview.md`, "Testing", with the keypad and
      by touch; the design system adaptation (the status line in two caption lines, the ring around
      the web view, the dimmed and pressed buttons); the keyboard's hide key on the Wi-Fi password
      form; no frames while the display is off with a page open; and the time and memory the first
      visit of the Settings submenu takes to load the library.
      Verify by noting the results in the pull request.
- [ ] 1.16 Add the note for users to `docs/webview.md` and to the web view's `CHANGELOG.md` entry: the
      web view is meant for pages made for the remote's screen and input, not as a full browser, and
      heavy sites load slowly and use much memory. Verify with the docs link check and by reading the
      entry.
- [ ] 1.17 Name the runtime library in the project context once ADR 0021 is accepted: the context of
      `openspec/config.yaml` (what is read from disk) and the project overview of `AGENTS.md`. Verify
      that `npx @fission-ai/openspec@1.14.1 instructions proposal --change web-view` shows the new
      context without a parse warning.

## 2. Phase 2: dialogs and logins of the page (`feat/webview-dialogs`)

Needs phase 1.

- [x] 2.1 Update the header copy to library 0.7.2; add `WebViewDialogRequest`, `WebViewAuthRequest`
      and the `WebViewRequests` table (`src/webview/webviewrequest.{h,cpp}`, registered in the
      `remote-ui.pro` block and in `test/webview/CMakeLists.txt` with the new target
      `testWebViewRequests`); set the request callbacks only with a 0.7.0 library; answer unhandled
      requests, unknown types and schemes at once; answer a waiting leave-page question before the item
      navigates (D18). Verified with `testWebViewRequests` (answers, a second answer refused,
      cancellation and a late answer, deletion, the kept user name) and `testWebViewItem` (defaults,
      the 10000 character cut, the view freed with requests pending, new windows, refused responses,
      `closeRequested()`, the trace without texts or credentials).
- [x] 2.2 Add `src/qml/components/WebViewDialog.qml` and `WebViewLogin.qml` to
      `resources/qrc/main.qrc` and show them on the developer page one at a time, script dialogs first;
      the notifications for refused content, refused windows, `window.close()` and a withdrawn login;
      their strings with translator comments in `en_US.ts` (D19, D21). Verified with the dialog and
      login cases of `testWebViewKeyboard` (OK, BACK, HOME, the d-pad, taps, the queue, the
      notifications, `window.close()`).
- [x] 2.3 Open the leave-page question on Stay, answer no on HOME, keep the user name for the retry
      of a rejected login, and open every login with the password masked (D19, D20). Verified with
      `testWebViewKeyboard` and `testWebViewRequests`.
- [x] 2.4 Put the dialogs on the design system: `Components.Sheet` and `Components.Dim`, the
      secondary button variant, `Components.Divider`, the 20 px gutter (D19). Verified:
      `./design-check.sh` reports no problem on any commit, and `testWebViewKeyboard` runs without the
      undefined-colour warnings it had before the adaptation.
- [x] 2.5 Document the requests, the dialogs, their keys and the device test with its test page in
      `docs/webview.md`. Verified with the docs link check: no error.
- [x] 2.6 Add the `CHANGELOG.md` entry under Unreleased "Added" in the commits that make the changes.
      Verified by reading the entries of each commit.
- [x] 2.7 Run the gates on every commit of the branch: `make test` (42 of 42), `make linux`,
      `./cpplint.sh` and `./design-check.sh`; at the tip also the icon font checks and `make ucr2`
      (48.3 MB, unstripped); `make linux` with `CONFIG+=no-webview` builds without any web view
      object.
- [ ] 2.8 Run "Device test: dialogs and logins" of `docs/webview.md` with the branch as rebased onto
      `main`, on a Remote Two and a Remote 3: the bottom sheet over the dimmed page, the prompt with the
      keyboard, the login and its retry, the notifications, and step 12, the journal search for every
      typed and shown text. Verify by noting the results in the pull request.

## Workflow follow-up

- The answers to the Open Questions of design.md can add tasks to phase 1 or 2 (ADR 0012, ADR 0004)
  or a requirement to `platform-constraints` (the memory budget, the form of redacted addresses).
- Archive `web-view` once both pull requests are merged and the open tasks are done, so that the
  `web-view` capability and the added requirements land in `openspec/specs/`.
- With the archive, point the link to this change in `docs/webview.md` at
  `openspec/specs/web-view/spec.md`: the change folder moves to the archive, and the docs link check
  would fail on the old link.
