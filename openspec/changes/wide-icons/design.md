## Context

The Current State Analysis was measured on `origin/main` at `697c664` (release 0.85.0, which
contains `svg-custom-icons`). Qt behaviour was measured with `qmlscene` of the shared Qt 5.15.19 on
macOS. The harness used `Icon.qml` unchanged and `ButtonIcon.qml` with only its imports stubbed. The
Qt source quoted is `qtdeclarative` 5.15.19.

**Page buttons.** Activity and remote pages build their buttons the same way.

- `Activity.qml:117-142` and `Remote.qml:20-60` create one `ButtonIcon.qml` per `text` or `icon`
  item.
- Cell size: `gridSizeW = uiPages.width / grid.width` and `gridSizeH = uiPages.height /
  grid.height`, with a default grid of 4 × 6 (`Activity.qml:119-122`, `Remote.qml:24-27`).
- Button: x = `gridSizeW × location.x`, width = `gridSizeW × size.width` (default 1), and the same
  for y and height. There is no spacing between cells.
- `uiPages` is the screen width (480 px) by the screen height minus the 80 px title
  (`Activity.qml:472-474`, `:543-545`; `BaseTitle.qml:11`). On a Remote 3 with a 4 × 6 grid a cell
  is 120 × 120 px; on a Remote Two it is 120 × 129 px.
- `ButtonIcon.qml` is used only by these two screens (`main.qrc:18`).

**How `ButtonIcon` draws an icon.**

- `ButtonIcon.qml:53-62`: the `Icon` gets the whole button as its box (`width`/`height` = the
  button), and `size` = 0.8 × the shorter side.
- `size` drives only the glyph: `Icon.qml:63` uses `pixelSize = size / 2`, which is 40 % of the
  shorter side. It also drives the placeholder glyph of a failed image (`Icon.qml:59`).
- An image icon uses the whole box: `Icon.qml:35-48` sets `Image` to `parent.width/height`,
  `PreserveAspectFit`, `sourceSize` = the box and `anchors.centerIn`.
- So an image icon is drawn edge to edge, as large as its aspect ratio allows, centred. A glyph is
  drawn at 40 % of the shorter side.
- The text label (`ButtonIcon.qml:64-75`) has the same full-button box. When an item has both text
  and an icon, the two overlap.
- Pressed state: the button background turns off-white (`:37-51`).
- No `Selectable` is used, and the buttons are not reached by the d-pad. On these screens
  DPAD_LEFT/RIGHT switch pages, and the other keys follow the button mapping.

**Measured layout (painted size) of SVG icons on buttons. It already matches the request:**

Harness cells of 120 × 100 px per grid unit (on a Remote 3 with the default 4 × 6 grid a cell is
120 × 120 px):

| icon | cell | drawn | raster (decoded) |
|---|---|---|---|
| 1:1 | 120 × 100 (1 × 1) | 100 × 100 | 120 × 120 |
| 1:1 | 240 × 100 (2 × 1) | 100 × 100 | **240 × 240** |
| 2:1 | 240 × 100 (2 × 1) | 200 × 100 | 240 × 120 |
| 4:1 | 480 × 100 (4 × 1) | 400 × 100 | 480 × 120 |
| 2:1 | 120 × 100 (1 × 1) | 120 × 60 | 200 × 100 |
| 1:2 | 240 × 100 (2 × 1) | 50 × 100 | **240 × 480** |
| 1:4 | 480 × 100 (4 × 1) | 25 × 100 | **480 × 1920** |
| 4:1 | 480 × 200 (4 × 2) | 480 × 120 | 800 × 200 |
| 1:4 | 120 × 400 (1 × 4) | 100 × 400 | 120 × 480 |
| 4:1 | 480 × 800 (whole page) | 480 × 120 | **3200 × 800** |

**Why the raster is larger than the drawing.** `qquickpixmapcache.cpp:441-443` decodes at
`QQuickImageProviderWithOptions::loadSize` (`qquickimageprovider.cpp:684-716`). With a fit or crop
fill mode, `loadSize` takes the *larger* of the two scale ratios (`:708`,
`preserveAspectCropOrFit && hr > ratio`). The image is therefore decoded to *cover* `sourceSize`.
`PreserveAspectFit` then scales it down to fit when it is painted.

- For a square icon in a square box both ratios are the same, so nothing is lost.
- For any other combination the raster is larger than drawn, by the aspect mismatch. The worst
  case on the default 4 × 6 grid is a 4:1 icon on a 1 × 6 button of a Remote 3 (120 × 720 px):
  2880 × 720 px, about 8 MB, drawn at 120 × 30.
- The downscale has no mipmaps, so a large factor also costs sharpness.
- This applies to every `Icon` with a non-square SVG, also in square boxes: a 4:1 icon in a 100 px
  tile is decoded at 400 × 100. So the `svg-custom-icons` rule "rasterized at the box size" holds
  only for square SVGs.
- With only `sourceSize.height` set, `loadSize` uses the height ratio alone, so the raster is
  `height × aspect` wide (a 4:1 icon in 120 × 100 gives 400 × 100). That is also larger than drawn.

**Size Qt reports for an SVG** (the `Image` implicit size without `sourceSize`, which is
`QImageReader::size()`):

- With no root `width`/`height`, `auto`, or `%`: the `viewBox`. 200 × 100 gives 200 × 100.
- With `px`/unitless or `mm`: the root `width`/`height`. `20mm × 10mm` gives 70 × 35, the same
  aspect.
- With `em`: the `viewBox` (the `em` values are ignored).
- When root `width`/`height` disagree with the `viewBox` (viewBox 200 × 100, width = height = 100),
  Qt reports 100 × 100 (`QImageReader::size()` and `QSvgRenderer::defaultSize()`; `viewBoxF()` is
  200 × 100). It does **not** letterbox: it **stretches** the viewBox content onto whatever size it
  rasterizes at, so a circle in the 2:1 viewBox is drawn as an upright ellipse in a square. Measured
  with a 200 × 200 `PreserveAspectFit` `Image`: implicit and painted 200 × 200, circle squashed. The
  reason is in Qt SVG 5.15 `qsvgtinydocument.cpp:470-480`: the image plugin's `QSvgRenderer` keeps
  the default `preserveAspectRatio() == false` (`qsvgtinydocument_p.h:132`), so the viewBox is
  scaled with separate x and y factors. The file's own `preserveAspectRatio` attribute is not read.
- The other way round, a requested size with the viewBox ratio draws the content undistorted:
  `QImageReader::setScaledSize(256 × 128)` on that file returns a 256 × 128 image with the viewBox
  mapped onto it.

So the reported size is the ratio Qt draws only when nothing else is requested; the viewBox ratio is
the only ratio that never distorts the artwork (D9). A user's 2:1 flag declared `width="100px"
height="100px"` was drawn on a 2 × 1 button (276 × 128) as a squashed 128 × 128 square, by the code
on `main` and by the first version of this change alike.

**Other users of a user-chosen icon** (all with a square box `width = height = size`, so a wide SVG
is drawn letterboxed, fitted to the width):

- entity tiles `Base.qml:461` (100) and the activity bar `Page.qml:655` (90);
- entity screen titles `BaseTitle.qml:17` (70) and the activity header `Activity.qml:479` (70);
- the activity menu and Fix states lists `Activity.qml:797`, `:859` (80);
- the icon selector `IconSelector.qml:290`, `:344` (the cell width);
- the profile `Profile.qml:342` and `MainContainer.qml:537` (80);
- the turn-off menu `PopupMenu.qml:192` via `Page.qml:582`;
- the slider widget min/max icons `SliderWidget.qml:489`, `:675`;
- the number entity `Number.qml:187`, integration icons `Info.qml:137` and `Configure.qml:185`,
  and `ConnectionStatus.qml:175`.

**Resource lookup.**

- `Resources::getIcon` (`resources.cpp:109-120`) → `getResource` (`:471-509`) → `getIconFile`
  (`:453-469`): one `QFile::exists` and, for SVG, `ImageDecoder::checkSvgFile` (a `stat` and a
  2-byte read).
- It returns `file:<path>`, or a glyph string. It knows nothing about the image's size.
- `ImageDecoder` (`imageDecoder.h:22-52`) holds the SVG limits (256 KB, no gzip).

**Tests.** `testIconFiles` (`test/ui/test_icon_files.cpp`) writes SVG fixtures into a temporary
resource directory and covers the lookup. `testImageDecoder` covers the decoder. No unit test loads
`Icon.qml`. The `svg-custom-icons` change checked `Icon.qml` with a `qmlscene` harness.

## Goals / Non-Goals

**Goals:**

- The page-button layout of image icons, which is already right, is written into the specs.
- An SVG icon is rasterized at the size it is drawn, never at the cover size.
- The aspect-ratio lookup and the fit computation are unit-tested C++ (ADR 0012).

**Non-Goals:** see proposal.md, "Non-Goals".

## Decisions

The maintainer decided D1–D15 on the proposed change.

### D1: The layout of page buttons stays as it is

The box is the whole button, and an image icon is fitted into it with its aspect ratio kept.
*Rationale:* the measured behaviour already shows what the user expects.

The glyph stays at 40 % of the shorter side. Glyphs are drawn too small today, but enlarging them
is a separate, later enhancement.

*Alternative:* derive the icon box from `size` (0.8 × the shorter side, a square). This would shrink
every image icon and bring back the square box. Rejected.

### D2: No padding

Image icons stay edge to edge. *Rationale:* "use the full available space"; no new design token is
needed.

### D3: Centred

An icon smaller than its button stays centred on both axes (`anchors.centerIn`, as today).
*Rationale:* it is what the user sees today.

### D4: Text and icon on one item are out of scope

*Rationale:* an item cannot have both today, so the overlap in `ButtonIcon.qml:53-75` cannot occur.

### D5: The raster fix is in `Icon.qml`, for every SVG icon

`Icon.qml` sets the `Image` width and height, and `sourceSize`, to the largest rectangle with the
icon's aspect ratio (D9) that fits the box, with `fillMode: Image.Stretch`. Without a fit or crop
mode, `loadSize` returns the requested size of a scalable format unchanged
(`qquickimageprovider.cpp:693-694`), so the raster equals the drawn size exactly, and Qt maps the
viewBox onto it. `PreserveAspectFit` would compute the cover size from the size the file declares:
when that ratio differs from the viewBox, the content is squashed again. Stretch distorts nothing
here, because the item already has the icon's ratio, up to rounding to whole pixels. When the size is
not known, `Icon.qml` keeps `PreserveAspectFit` and the box (D8).

*Rationale:* the drawn result is identical everywhere, and the `svg-custom-icons` rule "rasterized
at the box size" then also holds for non-square SVGs. *Alternative:* page buttons only. That would
leave the cover raster in tiles and the selector. Rejected.

### D6: Other icon boxes stay square

The places listed in Context keep their square box: entity tiles, the activity bar and header,
titles, the icon selector, the profile, slider icons, the number entity, the integration pages and
the lists. A wide icon is fitted to the width there, and the raster fix of D5 applies.
*Rationale:* keeps the change small; the drawn result is unchanged.

### D7: The aspect ratio comes from a cached C++ lookup in `Resources`

- `Resources::getIconSize(id, boxWidth, boxHeight)` (`Q_INVOKABLE`) reads the root `viewBox` of a
  `custom:` or `ctv:` SVG that passes `checkSvgFile` (D9). It reads only the root element, with
  `QXmlStreamReader` from Qt Core: `QSvgRenderer` would add QtSvg to the dynamic builds, which load
  the SVG image plugin but do not link the module. The four values may be separated by white space
  and / or commas.
- Without a usable viewBox (missing, not four numbers, a width or height that is not positive), it
  takes the size Qt reports, `QImageReader::size()`. When that cannot be read either, the size is
  invalid (D8).
- It caches the size per file path, keyed by modification time and file size.
- It returns `ImageDecoder::fitSize(source, box)`: the largest size with the source's aspect ratio
  inside the box.
- It returns an invalid size for a glyph, a bitmap, a missing or rejected file, an unreadable size
  or an empty box.

*Rationale:* exact on the first decode, testable C++ (ADR 0012).

Alternatives considered:

- **Read `Image.implicitWidth/Height` after a first decode, then re-request.** Every non-square SVG
  is decoded twice, and the first decode is still the cover raster. Rejected.
- **Set only `sourceSize.height`, or leave `sourceSize` unset.** Height only still decodes
  `height × aspect` wide. Unset decodes at the declared size, which is unbounded. Rejected.
- **An `image://` provider decoding through `ImageDecoder`.** It changes every icon URL and the
  cache keys. Too large. Rejected.
- **Only specify the layout and keep the cover raster.** Wasteful and less sharp. Rejected.

### D8: When the size cannot be read, fall back to the box

`Icon.qml` requests the box as today, and a file that then fails to decode shows the placeholder.
*Rationale:* no new failure path (ADR 0018).

### D9: The aspect ratio of an SVG icon is the ratio of its viewBox

The root `width` and `height` are ignored for sizing; only the ratio of the root `viewBox` counts,
not its absolute size. A 2:1 viewBox declared `width="100px" height="100px"` is drawn 2:1,
undistorted.

*Rationale:*

- The core requires a viewBox, and the aspect ratio check of remote-core and of the web configurator
  uses the viewBox only.
- Qt maps the viewBox onto whatever size is requested (see Context), so the viewBox ratio is the
  only ratio that never distorts the artwork.
- Real files often carry a width and height that disagree with the viewBox: written by generators
  or edited by hand.

This replaces an earlier decision: "the UI follows what Qt draws; such files are rejected at upload".
That follow-up for remote-core and the web configurator is dropped. A file whose width/height
ratio differs from the viewBox is drawn correctly, so there is nothing to reject.

### D10: No clamp of the aspect ratio

An SVG outside 1:4 to 4:1 (for example a file copied by hand) is fitted like any other.
*Rationale:* the core validates the range (ADR 0005, ADR 0018).

### D11: The maximum drawn size is the button

There is no extra cap. On a Remote 3 with the default 4 × 6 grid, a 4:1 icon on a whole-page button
(480 × 720 px) is drawn and rasterized at 480 × 120. *Rationale:* the raster is never larger than
the box.

### D12: No tint

SVG icons keep their own colours. *Rationale:* a non-goal of `svg-custom-icons`, unchanged.

### D13: Pressed feedback is out of scope

An opaque icon that fills its button hides the off-white pressed background, as square icons on
square buttons already do. *Rationale:* not part of the request.

### D14: Names

`Resources::getIconSize(id, boxWidth, boxHeight)` and `ImageDecoder::fitSize(source, box)`.
*Rationale:* the lookup sits next to `getIcon`; the arithmetic sits next to the other image limits.

### D15: SVG only, and the grid is unchanged

Bitmaps keep `sourceSize` = the box (product decision; the core's PNG and JPEG icons are square). The
default grid stays 4 × 6, as the code and the specs say.

## Risks / Trade-offs

Failure Mode Analysis (the change touches image loading, which is shared by about 200 `Icon` uses):

- [The lookup reads the SVG once more, synchronously on the GUI thread] → It reads only up to the
  root element; only a file without a usable viewBox is parsed in full by `QImageReader::size()`.
  The core caps icon uploads at 32 KB and the UI at 256 KB. The result is cached per file, so each
  file is read once per session. The lookup time is not measured; task 4.2 checked on a Remote 3
  that a 32 KB icon is shown sharp.
- [The binding calls the lookup before the box has its final size (0 × 0)] → An empty box returns
  an invalid size, which means "use the box". The binding re-evaluates when width and height change.
- [The cached size is stale after a file is replaced under the same name] → The cache is keyed by
  modification time and size. Task 2.3 covers it.
- [The lookup fails (broken file)] → The box is requested as today, and the decode failure shows
  the placeholder (D8).
- [A change in `Icon.qml` affects screens outside the page buttons] → The drawn result is unchanged
  by construction. The visual check covers a tile, the selector and a page button.

**Resource impact.**

- Memory per SVG icon goes down, to at most the box area. One cached size per SVG file is added,
  which is negligible.
- There are no new timers, polling, animations or Qt modules, and no effect on the frame rate or
  the input latency.
- CPU: one extra parse per SVG file and session.
- The binary grows by a few hundred bytes.

## Migration Plan

1. Implement as in `tasks.md`.
2. Verification target: the desktop simulator (`UC_MODEL=DEV`, against the core-simulator). The
   layout and the raster can be seen and logged there. Icon files are read from
   `<UC_RESOURCE_PATH>/Icon/`, so the fixture SVGs are copied there, and an activity page and a
   remote page are configured with them.
3. A device check (Remote Two or Remote 3) for the parse cost of the lookup and for the sharpness
   on the real panel.
4. No d-pad behaviour changes. The page buttons are not reached by the keypad.
5. Rollback: revert the commit. No data format changes.
