Phase 1 depends on nothing in this repository; its unit tests run without the core. Phases 2 and 3
are file-disjoint and both depend on phase 1: phase 2 calls `resource.getPageBackground()`, phase 3
calls `resource.getBackgroundImageClip()`. Once phase 1 is done, phases 2 and 3 can run in parallel. Phase 4 (design system document) is
file-disjoint from all others and can run any time. Phase 5 needs all phases and a
core-simulator build with the remote-core change `ui-page-background` (which itself needs the
core-api change "UI page background"). One commit per phase; the `CHANGELOG.md` entry goes into
the phase 2 commit, which makes the feature visible.

## 1. Background normalisation in Resources (C++)

- [x] 1.1 In `src/ui/resources.h/.cpp`, add `Q_INVOKABLE QVariantMap getPageBackground(const QVariant& background)`
      returning `{source, scale, align, dim}`: `source` from `getResource(BackgroundImage, image)`
      (empty for a missing `background`, a missing, empty or malformed `image` or a missing file);
      `scale` one of `fill` / `fit` / `none`, unknown or missing → `fill`; `align` one of `top` /
      `center` / `bottom`, unknown or missing → `center`; `dim` as `qreal` 0.0–0.8 from the
      percentage, clamped, missing or non-numeric → 0.4. `background: null`, a missing or empty
      `image` give an empty source without calling `getResource()`, so no "missing ':'" warning
      is logged (`resources.cpp:568-571`). Log unknown values at debug level in `lcResources`.
- [x] 1.2 In `src/ui/resources.h/.cpp`, add `Q_INVOKABLE QRect getBackgroundImageClip(const QString& id)`:
      resolve the file as `getResource(BackgroundImage, id)` does, read only its size with
      `QImageReader::size()` (no `read()`), and return `QRect(0, 0, w, qRound(w * 275.0 / 480))`
      when `h * 480 > w * 275`, otherwise an empty `QRect`; a missing file, an invalid size or a
      read error gives an empty `QRect`, and an empty id returns at once without a warning. Cache
      the size per path by modification time and file size, as `getIconSize()` does with
      `m_svgIconSizes` (`resources.cpp:168-209`, `resources.h:133-138`).
- [x] 1.3 Extend `test/ui/test_resources.cpp`: a temporary resource directory with a
      `BackgroundImage/bg.png`; cases: no background (`QVariant()`, empty map) → empty source and
      defaults; `custom:bg.png` resolves to `file:<dir>/BackgroundImage/bg.png`; missing file and
      identifier without colon → empty source; every `scale` and `align` value passes through;
      unknown `scale` / `align` → defaults; `dim` 0, 60, 80 → 0.0, 0.6, 0.8; -5 → 0.0; 95 → 0.8;
      `"abc"` and missing → 0.4.
- [x] 1.4 Size probe cases in `test/ui/test_resources.cpp`: generate PNG and JPEG files of
      480 x 275, 480 x 800 and 480 x 854 in the temporary `BackgroundImage` folder; 480 x 275 →
      empty rect; 480 x 800 and 480 x 854 → `QRect(0, 0, 480, 275)`; a missing file, an empty id
      and a file with garbage content → empty rect; a file replaced by one of another size is
      probed again (cache invalidated by modification time or size).
- [x] 1.5 Check `testResources` in `test/ui/CMakeLists.txt` still builds with the new code
      (`resources.cpp` is already compiled in; add sources only if the new code needs them).

## 2. Background layer in the activity and remote screens (QML, depends on 1.1)

- [x] 2.1 Add `src/qml/components/UiPageBackground.qml`: a full-size `Item` with a `background`
      property (the raw page background), two `Image`s (`asynchronous: true`, `cache: true`, no
      `sourceSize`, `horizontalAlignment: Image.AlignHCenter`) each with a scrim
      `Rectangle` in `colors.bg` at `opacity: dim` (tokens only, `./design-check.sh` checks this
      file); maps `scale` to `fillMode` (`fill` → `PreserveAspectCrop`, `fit` →
      `PreserveAspectFit`, `none` → `Pad`) and `align` to `verticalAlignment`; on a change of the
      normalised value (compare source, scale, align, dim) load it into the hidden layer and, when
      that layer's status is `Ready` or its source is empty, fade it in and the other out over
      200 ms; ignore a status change of a layer whose values are no longer the latest target.
- [x] 2.2 Register `UiPageBackground.qml` in `resources/qrc/main.qrc` (`alias=` with the path back
      to `../../src/qml/components/UiPageBackground.qml`, next to the other components).
- [x] 2.3 `src/qml/components/entities/activity/deviceclass/Activity.qml`: add
      `Components.UiPageBackground` as the first visual child, filling the screen, with
      `background` bound to `pages[uiPages.currentIndex].background`, guarded against an
      `undefined` page (no pages, index past the end). Insert it before the title `Rectangle`
      (`:501`), so the title, the `PathView` with the web view widget, the `PageIndicator` and the
      `DropDownMenu` sheet are drawn over it; leave `parsePageItems()` and the widget untouched.
- [x] 2.4 `src/qml/components/entities/remote/deviceclass/Remote.qml`: the same as 2.3, inserted
      before `BaseTitle` (`:160`).
- [x] 2.5 `CHANGELOG.md`, "Unreleased", "Added": activity and remote pages show a full-screen
      background image set in the web-configurator, with fill / fit / original-size scaling,
      alignment and dimming; main pages can use a full-screen image too, showing its top
      480 x 275 part.
- [ ] 2.6 `src/qml/components/NumberWidget.qml`: the minus and plus buttons have no fill at rest
      (`colors.transparent`) and keep the pressed inversion (design D11); `CHANGELOG.md`,
      "Unreleased", "Changed".

## 3. Main page header clip (QML, depends on 1.2)

- [x] 3.1 `src/qml/components/Page.qml`, `headerImage`: add a `readonly property rect headerClip:
      resource.getBackgroundImageClip(pageImage)` (not `clip`, which is an `Item` property). For a
      non-empty clip, decode at the header's scale for a 480 x 275 image, `s = max(width / 480,
      height / 275)`: `sourceSize: Qt.size(round(480 * s), 0)` and `sourceClipRect: Qt.rect(0, 0,
      round(480 * s), round(275 * s))` (design D8); otherwise `sourceClipRect` is `undefined` and
      `sourceSize` the header size, as today. Nothing else changes, so a 480 x 275 image keeps
      today's centre `PreserveAspectCrop` with the header-bound `sourceSize`.

## 4. Design system (docs, independent)

- [ ] 4.1 `docs/design-system.md`: amend principle 4 (`:42-43`) with the user-set scrim over a
      page background (0–0.8, default 0.4, set per page in the web-configurator), and add
      `Components.UiPageBackground` to the component table of section 7.
- [ ] 4.2 `docs/design-system.md`, "Slider items": the minus and plus buttons of a number item have
      no fill at rest and invert when pressed (design D11).

## 5. Verification

- [ ] 5.1 `make test` (or `ctest -R testResources` in `test/build`) passes.
- [ ] 5.2 Format the changed lines (`git add -u && git clang-format && git add -u`) and run
      `./cpplint.sh` and `./design-check.sh`.
- [ ] 5.3 `make linux` and `make ucr2` (QML changed), then `git checkout resources/translations/`
      and leave the generated `.qm` files uncommitted.
- [ ] 5.4 Desktop walk with `UC_MODEL=DEV` against a core-simulator with the remote-core change,
      backgrounds set over REST: an activity with three pages (`fill`; `fit` + `top`; `none` +
      `bottom` with `dim` 0, two pages sharing one image): full-screen image behind title, buttons
      and page indicator; cross-fade on swipe; no visible change between the pages that share an
      image; PATCH the current page's background while it is shown (fades to the new one); delete
      the image file (black after the next page change); activity in Error (red title over the
      image); remove the background with `background: null` (fades to black); an activity page with
      a web view widget and a background, where the web engine is available (widget drawn over the
      image, loads and unloads as before); a remote with one page with a background; a main page with a 480 x 275 image,
      compared with a screenshot of the base build (`cfaaa8a`): identical in the 260 px header and
      with the activity bar; a main page with a 480 x 800 image: its top 480 x 275 part, rendered
      like a 480 x 275 image, also with the activity bar.
- [ ] 5.5 On a Remote Two and a Remote 3: swipe and DPAD_LEFT / DPAD_RIGHT through pages with
      backgrounds (keypad walk), watch for dropped frames, and compare the UI's resident memory
      with and without backgrounds (`platform-constraints` "Memory usage", "Rendering frame rate").
- [ ] 5.6 Run `openspec validate ui-page-background --strict` (OpenSpec CLI 1.14.1); the deltas are synced into the
      living specs when the change is archived.
