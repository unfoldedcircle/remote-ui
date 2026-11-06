## Why

Custom icons and TV channel icons are 90 x 90 PNG or JPG bitmaps today, which blur wherever an icon
is drawn larger than that and are bigger on disk than the vector originals most icons come from.
SVG icons scale to every box without loss. The rendering stack already decodes SVG on every target
(the start-up spinner is one), so what stands between the user and SVG icons is the UI's file
name handling, the core's upload validation and the web configurator's cropper. **The implementation is on the branch `feat/svg-icons`.**

At the same time the images the UI decodes from sources it does not control — media artwork
from integrations, the base64 image of an integration setup page — are decoded at whatever size
the data declares. For a bitmap the file bounds that; for an SVG the declared size does, and a
gzip-compressed SVG is inflated without any limit. Accepting SVG anywhere means bounding those
decodes first.

## What Changes

- A custom icon (`custom:<file>`) or TV channel icon (`ctv:<file>`) may name an `.svg` file next
  to `.png`, `.jpg` and `.jpeg`; the file name is used as is, whatever the extension. The icon
  selector's Custom Icons tab lists SVG files too.
- An SVG icon is rasterized at the size of the box it is shown in, never at the size it declares.
- An icon file that exists but cannot be used shows the placeholder icon (question mark in a
  circle) and is logged, instead of leaving an empty box: a compressed `.svgz`, an SVG above
  256 KB (rejected before the image loader sees it, and not offered in the icon selector), or any
  file the image loader fails to decode. A missing file still leaves the box empty, as before.
- Media artwork and integration images are decoded within fixed limits by one decoder: a bitmap
  larger than the 1024 px box is downscaled while decoding, an SVG is rasterized to fit the box,
  and compressed SVG data, an SVG above 256 KB, an SVG without a size and a bitmap above 32
  megapixels are rejected as undecodable and logged.
- The UI does not validate SVG content beyond that. The core validates uploads (element
  allow-list, size, structure); the UI's checks only bound what a file can cost.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `ui-resources`: icon identifiers (any image file name), missing and default icons (an
  unusable file shows the placeholder), icon rendering (SVG rasterized at the box size, a failed
  decode shows the placeholder), the icon selector (SVG files listed, rejected ones not).
- `media-player`: artwork download (bounded decoding: SVG rasterized to fit, compressed and
  oversized data rejected).

## Impact

- **Hardware models:** both. No hardware-specific behaviour.
- **remote-core dependency:** none for the UI change itself. For a user to *get* an SVG icon
  onto the remote, the core must list `svg` in the `file_formats` of the `Icon` and
  `TvChannelIcon` resource types of `GET /api/resources` and accept it in
  `POST /api/resources/{type}`, and the web configurator must upload SVG files without its
  canvas cropper. Until then this change only hardens the decoders and changes nothing visible.
- **Third-party assets or libraries:** none. The Qt SVG module and its image format plugin are
  already linked into every static build (`QT += svg` in `remote-ui.pro`).
- **Code:** `src/ui/imageDecoder.{h,cpp}` (new), `src/ui/resources.{h,cpp}`,
  `src/ui/entity/mediaPlayer.cpp`, `src/qml/components/Icon.qml`, `remote-ui.pro`,
  `test/ui/CMakeLists.txt`, `test/ui/test_image_decoder.cpp` and `test/ui/test_icon_files.cpp`
  (new), `CHANGELOG.md`.
