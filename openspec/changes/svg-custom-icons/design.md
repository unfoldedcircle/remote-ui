## Context

Current State Analysis, measured on `main` before this change, against the living `ui-resources` and
`media-player` specs:

- **SVG already renders on every target.** `remote-ui.pro:3-6` adds `QT += svg` to every
  `CONFIG+=static` build; qmake's static plugin import then links the `qsvg` image format plugin
  because it is declared as extending the svg module (`QT_PLUGIN.qsvg.EXTENDS = svg`). The device
  toolchain (Qt 5.15.8), the Linux x64 image and the Windows x64 image (Qt 5.15.19) all build
  static. `src/qml/components/LoadingFirst.qml:47` draws `qrc:/images/quarter_arc.svg` at
  start-up, which proves the plugin is in the binary. Dynamic desktop builds load the plugin from
  the Qt installation.
- **The QML side sizes images correctly already.** `src/qml/components/Icon.qml:40-41` sets
  `sourceSize` to the box and uses `PreserveAspectFit`; `ImageLoader.qml:66-67` (artwork and
  media browser thumbnails) and `Page.qml:228-229` (background images) do the same. With a fit or
  crop mode Qt Quick rasterizes an SVG at the requested size with the aspect ratio kept
  (`QQuickImageProviderWithOptions::loadSize`), verified with a qmlscene test: a 200 x 100 SVG
  requested at 90 x 90 paints 90 x 45. The one QML image without a `sourceSize` is the
  driver-provided image of an integration setup page (`integrations/UserAction.qml:89-103`).
- **Only the C++ file name handling blocks SVG icons.** `src/ui/resources.cpp:113-146`
  (`getIcon`) strips and re-appends a `.png` / `.jpg` / `.jpeg` suffix — a no-op left over from
  an unimplemented "state suffix" feature — and `resources.cpp:461-463` (`getCustomIconList`)
  filters the icon directory to those three extensions. An `.svg` file in the directory is
  invisible to the selector.
- **Artwork is decoded unbounded.** `src/ui/entity/mediaPlayer.cpp:92` and `:128` call
  `QImage::loadFromData` on the downloaded or base64 data and only afterwards scale to 1024 px
  (`normalizeMediaImage`, `:31`). For an SVG the raster is allocated at the declared size first:
  20000 x 20000 is 1.6 GB. `QSvgIOHandler::canRead` accepts gzip data and
  `qt_inflateSvgzDataFrom` (Qt SVG `qsvgtinydocument.cpp`) inflates it with no cap, so a small
  compressed file can expand to anything. Nothing caps the pixel count of a bitmap either.
- **Outside this repository:** the core advertises `file_formats: [png, jpg]` for `Icon` and
  `TvChannelIcon` and validates the extension on upload; the web configurator derives its file
  picker from that list and re-encodes every image through a canvas cropper, which browsers
  cannot do for `image/svg+xml` (they fall back to PNG). The resource directory is read from
  disk, as ADR 0003 allows for firmware-provided data.

## Goals / Non-Goals

**Goals:** an `.svg` custom or TV channel icon renders wherever a `.png` one does, sharp at every
box size; no image the UI decodes from data it does not control can cost more than a fixed
amount of memory and parse work; an icon file that cannot be used is visible as such (placeholder
plus log line) rather than a silent hole; the limits live in one unit with tests.

**Non-Goals:** validating SVG *content* (element allow-list, scripts, external references) — that
is the core's upload validator; tinting SVG icons to the theme colour like
the glyph icons (a `ColorOverlay` per icon, one offscreen layer each, adds up in the 100-cell
selector grid); changing how the web configurator or the core handle uploads; a download size cap
for artwork over HTTP (the request already has a 15 s transfer timeout; a cap is a separate
decision).

## Decisions

- **D1 — The file name is used as is; no extension list in the UI.** `getIcon` no longer parses
  the suffix. Whatever the core lets through and the image plugins decode is an icon. The
  alternative, adding `.svg` to the list, would keep a list that has to be edited for every
  format and mirrors a decision the core already makes.
- **D2 — One bounded decoder for every decode the UI performs itself** (`uc::ui::ImageDecoder`,
  `src/ui/imageDecoder.{h,cpp}`): format detected from the data, gzip rejected up front, SVG
  capped at 256 KB and rasterized to fit the box via `QImageReader::setScaledSize`, bitmaps above
  32 megapixels rejected and larger ones downscaled while decoding, and a final scale for formats
  that do not report their size. The media player calls it for downloaded and for base64
  artwork. The alternative, keeping `loadFromData` and adding checks in the media player, would
  leave the next caller (a driver image, a dock picture) to repeat them.
- **D3 — For icon files the check is cheap and the image loader does the rasterization.** QML's
  `Image` already rasterizes at `sourceSize`; the UI cannot put a decoder in front of it without
  an image provider. So `Resources` only checks what the loader cannot bound — the file size and
  the gzip magic — and lets the loader decode. This costs one `stat` and a two-byte read per
  lookup; `getIcon` is called once per binding evaluation, so no cache is needed.
- **D4 — An unusable file shows the placeholder; a missing file stays empty.** A missing file is
  the documented normal case (an icon deleted in the web configurator) and stays as it was. A
  file that is there but rejected or undecodable is a defect the user should see and the log
  should name; the placeholder glyph is what the icon system already uses for "cannot draw". The
  QML side handles the second half: `Image.Error` switches the box to the placeholder glyph.
- **D5 — The limits are 256 KB per SVG and 32 megapixels per bitmap.** The core's upload limit
  for icons is 32 KB, so 256 KB never rejects a legitimate icon and still bounds a parse to a
  few milliseconds on the device. 32 megapixels is 128 MB decoded, a transient inside the 1 GB
  memory budget; a 4K cover (8 MP) or a phone photo (12 MP) still decodes. Both are constants
  of the decoder, tested and named in the spec.
- **D6 — The setup-page image stays unbounded, with the risk stated.** `UserAction.qml` shows
  the base64 image with `Layout.fillWidth` and no `sourceSize`. Giving it a `sourceSize.width`
  would, with `PreserveAspectFit`, *upscale* small bitmaps at decode time (Qt scales to the
  requested width whenever a fit mode is set), a visible change on a page the driver designs.
  The payload is capped at 32 KB by the Core-API and comes from a driver the owner installed.
  Left as an Open Question for the integration setup capability.

## Risks / Trade-offs

Failure Mode Analysis — the change adds a source file to the static build and touches the
artwork path:

- [New `.cpp` missing from `remote-ui.pro` or a test target] → registered in `HEADERS` /
  `SOURCES` and in every test target that compiles `resources.cpp` or `mediaPlayer.cpp`; the
  desktop build and `make test` fail loudly otherwise.
- [A bitmap format whose `QImageReader::size()` is unknown bypasses the pixel cap] → decoded
  fully, then scaled; the cap only misses formats without a size header, which the artwork
  sources (PNG, JPEG, WebP) all have.
- [An SVG that parses but takes long to rasterize (thousands of paths)] → bounded by the 256 KB
  cap: path data is roughly one command per eight bytes, and rasterization at 1024 px of such a
  file stays in the tens of milliseconds on the device, in the thread pool, off the UI thread.
- [The image loader itself receives a hostile SVG via a `custom:` icon] → the gzip and size
  checks run first; the loader rasterizes at the box size; a file that still fails shows the
  placeholder. Content validation is the core's (element allow-list, no `image`, `script`,
  `foreignObject`); the UI does not duplicate it.
- [A remote media browser thumbnail is a compressed SVG bomb] → loaded by QML's `Image` from
  the URL, where the UI has no hook before the plugin inflates it. Accepted: the source is an
  integration the owner configured, and the same server could just as well send a 100 MB JPEG.
- [Artwork that decoded before is now rejected] → only when it exceeds 32 megapixels or is
  compressed SVG; both were unusable on the device anyway (the former by memory, the latter by
  the API, which promises PNG or JPG).

Resource impact: no new timers, polling or Qt modules. Memory per artwork decode is now at most
128 MB transient (bitmap cap) or 4 MB (SVG at 1024 px) instead of unbounded; icon rasterization
memory is unchanged (box size). Binary size grows by the decoder unit, a few kilobytes. The per-
lookup file check on icons is one `stat` and a 2-byte read, negligible.

## Migration Plan

1. Merge this change. Behaviour is unchanged for every icon and artwork in use today except the
   two rejections above.
2. remote-core lists `svg` for `Icon` and `TvChannelIcon` and validates uploads;
   the web configurator uploads SVG files without the cropper. Both ship whenever ready; the UI
   needs no further change.
3. Verification: unit tests (`testImageDecoder`, `testIconFiles`) cover the decoder limits and
   the icon lookup; a qmlscene harness of `Icon.qml` covers the placeholder on a failed decode.
   The Remote-Core Simulator cannot upload SVG yet, so the end-to-end path (upload, selector,
   tile) is checked on a device or simulator once the core accepts SVG: copy an `.svg` into
   `<UC_RESOURCE_PATH>/Icon/` and pick it in the selector. No d-pad behaviour changes.

Rollback is reverting the commit; no data format changes.

## Open Questions

- The base64 image of an integration setup page (`UserAction.qml`) is still decoded at its
  declared size (D6). Bound it in a change to the integrations capability, either by decoding
  through `ImageDecoder` in C++ before handing a `image://` URL to QML, or by accepting the
  upscale of small bitmaps.
- Whether artwork downloads should have a byte cap in addition to the transfer timeout.
