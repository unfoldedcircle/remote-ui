# Tasks

The decisions are D1–D15 in design.md: the raster fix in `Icon.qml` for every SVG icon, a cached
C++ lookup in `Resources`, and the page-button layout unchanged. No new source files are added, so
no registration in `remote-ui.pro`, `.qrc` or the test CMake lists is needed.

Phases 1 and 2 touch different files and can run in parallel. Phase 3 needs both.

## 1. Fit computation (`ImageDecoder`)

- [x] 1.1 Add `ImageDecoder::fitSize(QSizeF source, QSizeF box)` to `src/ui/imageDecoder.{h,cpp}`.
      It returns the largest size with the source's aspect ratio inside the box, rounded to whole
      pixels, and an invalid size for an empty source or box. Verify with `testImageDecoder` cases
      for 1:1, 2:1, 3:1, 4:1, 1:2 and 1:4 in the Remote 3 default-grid boxes 120 × 120, 240 × 120,
      480 × 120, 480 × 240, 120 × 480 and 480 × 720 (whole page), including the scenario values in the `activities` delta (for example 1:1 in 240 x 120 gives
      120 x 120).

## 2. Size lookup (`Resources`)

- [x] 2.1 Add `Q_INVOKABLE QSize getIconSize(const QString& id, int boxWidth, int boxHeight)` to
      `src/ui/resources.{h,cpp}`. For a `custom:` or `ctv:` SVG that passes
      `checkSvgFile`, it reads the size of the root `viewBox` (D9), falling back to
      `QImageReader::size()` without a usable viewBox, caches it per path with the modification time
      and file size, and returns `ImageDecoder::fitSize(...)`. For a glyph, a bitmap, a missing or
      rejected file, an unreadable size or an empty box, it returns an invalid size. Verify with
      task 2.2.
- [x] 2.2 In `test/ui/test_icon_files.cpp`, write the fixture SVGs into the temporary resource
      directory:
      - viewBox only, at 1:1, 2:1, 3:1, 4:1, 1:2 and 1:4;
      - 2:1 with root width/height in unitless px, `mm`, `em`, `%` and `auto`;
      - a viewBox of 200 × 100 with width = height = 100.

      Check the returned sizes against the `ui-resources` scenarios (every unit gives 2:1, and the
      mismatch case is 2:1, the viewBox ratio). Run with
      `cd test/build && ctest -R testIconFiles`.
- [x] 2.3 Add a test that replaces a fixture with a file of another aspect ratio under the same name
      and expects the new size (cache invalidation), plus tests that a PNG, a `uc:` glyph, a missing
      file, a gzip SVG and an unparsable SVG return an invalid size.
- [x] 2.4 Take the aspect ratio from the viewBox (D9) and add fixtures for it: a 2:1 flag declared
      100px × 100px, a 1:1 viewBox declared 200 × 100, a fractional viewBox (`0 0 24.5 24`), a
      viewBox separated by commas, and the fallbacks without a viewBox, with three values and with a
      zero width. Add a `fitSize` case for the fractional size.

## 3. Rendering (`Icon.qml`)

Needs phases 1 and 2.

- [x] 3.1 In `src/qml/components/Icon.qml`, bind the `Image` `width`, `height` and `sourceSize` to
      the size from the lookup when it is valid, and to the whole box otherwise (today's
      behaviour). Use `Image.Stretch` when the size is known and `PreserveAspectFit` otherwise
      (D5); keep `anchors.centerIn` and the `Image.Error` placeholder.
      Leave `ButtonIcon.qml` unchanged. Verify with a `qmlscene` harness of `Icon.qml` and
      `ButtonIcon.qml` (stubbed `resource`, `colors`, `fonts`, `ui`) that the painted sizes of the
      design.md table stay the same and that `implicitWidth/Height` (the raster) now equal the
      painted size.
- [x] 3.2 Run `./design-check.sh` and `./cpplint.sh`. Format with `git clang-format`. All must be
      clean.
- [x] 3.3 Add a `CHANGELOG.md` entry under Unreleased, in the same commit as 3.1. Suggested
      "Changed": "SVG icons that are wider or taller than their box are rendered at the size they
      are shown: sharper and lighter on memory. Wide SVG icons on activity and remote page buttons
      use the whole button."

## 4. Verification

- [x] 4.1 Run the desktop simulator (`UC_MODEL=DEV`, core-simulator). Copy the files of [fixtures/](fixtures/README.md) into
      `<UC_RESOURCE_PATH>/Icon/` and configure an activity page and a remote page with
      `custom:<file>` icon items in 1 × 1, 2 × 1, 4 × 1, 4 × 2, 1 × 4 and 4 × 6 (whole page) sizes on the default
      4 × 6 grid.
      Compare against every scenario of the `activities` and `remote-entity` deltas. Also check a
      glyph item, a PNG item and a broken SVG (placeholder). Check a wide SVG on an entity tile and
      in the icon selector: it is unchanged, fitted to the width. The QML log is free of warnings.
- [x] 3.4 With the harness, check a 2:1 flag declared 100px × 100px in 276 × 128, 240 × 120,
      120 × 120 and 480 × 240 boxes: drawn 2:1, decoded at the drawn size, and its pixels equal a
      2:1 reference rendered by `QImageReader` with the viewBox mapped onto the size.
- [x] 4.2 On a Remote 3, look at the 32 KB 4:1 and 1:4 icons
      (`fixtures/size-32kb-4x1.svg`, `fixtures/size-32kb-1x4.svg`) on page buttons, up to the whole
      page, for sharpness. Verified: the 32 KB icon is sharp full screen. The lookup timing is not
      measured (design.md, Risks).
- [x] 4.3 Run `make test` (all targets green) and
      `npx @fission-ai/openspec@1.14.1 validate wide-icons --strict`.

## Workflow follow-up

- Archive `wide-icons` after the implementation, so its added requirements land in `ui-resources`,
  `activities` and `remote-entity`.
