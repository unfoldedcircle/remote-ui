## Context

### Current State Analysis (commit `cfaaa8a`, release 0.86.0)

**Main page background (the only background today)**

- `src/qml/components/Page.qml:236-274` is the header `Image`:
  - `source: resource.getBackgroundImage(pageImage)` (`:239`)
  - `sourceSize.width` / `sourceSize.height` bound to the header size (`:240-241`)
  - `fillMode: Image.PreserveAspectCrop` with the default centre alignment, `asynchronous: true`,
    `cache: true` (`:242-244`)
  - a black scrim at `opacity: 0.7` (`:248-252`), a bottom gradient (`:254-263`) and a rounded
    `OpacityMask` (`:265-273`) over it
  - `Page.qml` imports `QtQuick 2.15` (`:4`)
- The header (`:169-235`) is 260 px high. With the activity bar it grows to 440 px, or to 680 px
  with a media component (`:174-209`). Because `sourceSize` follows the height, the image is
  decoded again while the header animates.
- `Resources::getBackgroundImage()` (`src/ui/resources.h:46`) calls `getResource()`
  (`src/ui/resources.cpp:560-598`).
  - An id without `:` is logged as a warning and resolves to nothing (`:568-571`).
  - The `BackgroundImage` case (`:589-597`) ignores the prefix and returns
    `file:<UC_RESOURCE_PATH>/BackgroundImage/<name>` when the file exists.
  - The folder map is at `resources.h:141-144`.
  - Nothing is fetched over the Core-API: the core writes the files to the shared file system.
- A size probe already exists for SVG icons: `Resources::getIconSize()` (`resources.cpp:168-209`)
  reads the size once per file. It caches the size by modification time and file size
  (`SvgIconSize`, `m_svgIconSizes`, `resources.h:133-138`) and falls back to
  `QImageReader::size()` (`:197`). `ImageDecoder` (`src/ui/imageDecoder.h:24-60`) holds the
  decoding limits.

**Activity and remote UI pages**

- The UI pages are not modelled in C++. `options.user_interface` is kept as a raw `QVariantMap`
  (`src/ui/entity/activity.cpp:49-50` and `:260-261`, `src/ui/entity/remote.cpp:42-43` and
  `:102-103`) and exposed as `Q_PROPERTY(QVariantMap ui ...)` (`activity.h:115`, `remote.h:71`).
  QML is the only reader of `page_id`, `name`, `grid` and `items`.
- `Activity.qml` (985 lines) holds `property var pages: entityObj.ui.pages` (`:21`) and re-assigns
  it on `uiConfigChanged` (`:458-459`). `parsePageItems()` (`:119-253`) creates the items. Besides
  text, icon, media player, sensor, select, slider and number items, it now creates the web view
  widget `WebViewWidget.qml` (`:226-247`): one per activity, loaded only while its page is current
  and the screen is open (`pageShown`, `:243-246`). The screen has:
  - an 80 px title `Rectangle`, transparent, or `colors.red` while the activity is in Error or
    Timeout (`:501-570`), with the status cluster `TitleStatus` (`:565-569`)
  - the `PathView uiPages` below the title, with `cacheItemCount: 5` (`:572-599`)
  - a `PageIndicator` at the bottom (`:601-617`)
  - the `DropDownMenu` (`:619`), which is now a `Components.Sheet`, opaque `bg`
    (`src/qml/components/entities/DropDownMenu.qml:16-20`)
  - the page delegate `Item gridContainer` (`:761-797`)
- The web view widget is an ordinary scene-graph item: `WebViewItem` is a `QQuickItem` that draws
  the page as a texture in `updatePaintNode()` (`src/webview/webviewitem.h:159`, `:359`). Its
  placeholder is opaque `colors.surface` (`WebViewWidget.qml:59-62`). A layer behind the page grid
  therefore lies behind the widget like behind any other item; it needs no hole or underlay.
- `Remote.qml` is unchanged from the base: `BaseTitle` is a transparent 80 px `Item` (`:160`),
  followed by the `PathView` (`:166-193`), the `PageIndicator` (`:195-211`) and the delegate
  (`:213-249`). `pages` is re-assigned on `uiConfigChanged` (`:142-143`).
- Both screens are children of `BaseDetail` (`src/qml/components/entities/BaseDetail.qml:14-16`),
  a full-screen `colors.black` `Rectangle` that slides in with its `open` / `closed` states
  (`:22-33`). It is shown in the full-screen `containerSecond` popup
  (`src/qml/main.qml:511-513`), with no status bar over it. `BaseDetail` draws the close icon at
  `z: 1000` (`:180-186`) and the unavailable overlay (black, 0.85) at `z: 2000` (`:199-204`).
- The buttons (`src/qml/components/ButtonIcon.qml:28`) have a transparent background and
  off-white text and icon. They are hard to read on a bright picture.
- Screen size: `ui.width` / `ui.height` are the logical size. The Remote Two is 480 x 854,
  rotated by the UI; the Remote 3 is 480 x 800 (`src/ui/uiController.cpp:40-54`). The simulator
  (`DEV`) is 480 x 850.

**Design system**

- `docs/design-system.md` was accepted on 2026-10-07 (ADR 0021, which supersedes 0019).
  - Principle 4 (`:42-43`): "Opacity is for disabled controls (0.4), dim layers (0.85) and an
    unavailable entity only". A user-set scrim over a picture is not covered.
  - Colours are tokens; the page background is `bg` (`colors.bg`, `src/ui/colors.h:42`), and
    `Components.Dim` (`src/qml/components/Dim.qml`) is the 0.85 dim.
- `./design-check.sh` (`tools/design-check.py`) rejects literal colours and font sizes below
  22 px in `src/qml`. The entity detail screens (`src/qml/components/entities/*/*`) are on its
  allow-list (`tools/design-check-allow.txt`); a new component in `src/qml/components/` is
  checked.
- The `platform-constraints` "Legibility" requirement: a change that needs a colour or look the
  design system doesn't specify adds it to the design system in the same change.

**Tests**

- `test/ui/test_resources.cpp` (`testResources`, `test/ui/CMakeLists.txt:436-456`, which compiles
  `resources.cpp` and `imageDecoder.cpp`) covers link content, Markdown detection and the license
  blocks. No test covers `getResource()` for background images or a background size probe.

### Companion changes

- core-api, "UI page background":
  - adds the optional `background` object (`image`, `scale`, `align`, `dim`) to
    `ActivityUserInterfacePage` and its update schema, which activities and remotes share
  - allows the `BackgroundImage` sizes 480 x 275, 480 x 800 and 480 x 854
  - proposed REST 0.59.0 / WS 0.52.0
- remote-core, `ui-page-background`: validates, stores and sends `background`.
- web-configurator, `ui-page-background`: sets, edits and removes it.

### Constraints

- ADR 0005: the core owns the configuration; the UI only renders it.
- ADR 0012: logic belongs in C++. Normalising and validating the settings is logic; mapping
  normalised values onto `Image` properties is presentation.
- ADR 0009: the new logic gets unit tests.
- ADR 0018: unknown values from the core are tolerated.
- ADR 0008: the same behaviour on both models.
- ADR 0021 and the `platform-constraints` "Legibility" requirement: colours are tokens, and a
  look the design system does not specify is added to it in the same change.

## Goals / Non-Goals

**Goals:**

- Show the optional per-page background of activity and remote UI pages full screen, with the
  `fill`, `fit` and `none` scale modes, `align` and `dim`.
- Keep the buttons readable and the frame rate and memory within the budgets.
- Allow 480 x 800 and 480 x 854 images as main page backgrounds (their top 480 x 275 part), while
  existing 480 x 275 main page images look exactly as today.

**Non-Goals:**

- Editing a background on the remote: the web-configurator owns editing.
- The scale modes `fit_width` / `fit_height`, tiling and stretching. Every allowed image is 480 px
  wide, so the first two would behave like `none` and `fill`.
- Per-model image variants.
- Settings for main page backgrounds: main pages keep their fixed scrim and header.
- Dimming that follows the brightness of the image. The fixed `dim` value is set by the user.

## Decisions

### D1 — Image sizes are the core's business

The core accepts 480 x 275, 480 x 800 and 480 x 854 for `BackgroundImage`. The UI does not check
sizes; it renders any image it is given with the page's scale mode.

- Rejected: any size within bounds, which only makes sense together with `fit_width` /
  `fit_height`. The web-configurator crop editor already produces an allowed size from any photo.
- Rejected: a separate resource type for full-screen images. Main pages could not use them, and it
  would need a second gallery and a second backup budget.

### D2 — `background` is one object on the page

The page carries `background: { image, scale, align, dim }`. `image` uses the same identifier as
the main page `image` (`custom:<file>`). The core removes a background by setting
`background: null`. The UI also treats a missing `background`, or one without a usable `image`,
as no background.

- Rejected: flat fields next to `items`. Every later option would add another top-level field.

### D3 — Scale modes keep the aspect ratio

| `scale` | Qt `fillMode` | Effect |
|---|---|---|
| `fill` (default) | `PreserveAspectCrop` | covers the screen, crops the overflow |
| `fit` | `PreserveAspectFit` | the whole image is visible, the rest stays black |
| `none` | `Pad` | original size, cut off or framed in black |

`align` maps to `verticalAlignment` `AlignTop`, `AlignVCenter` or `AlignBottom`; the horizontal
alignment is always `AlignHCenter`. Examples:

- A 480 x 854 image on the Remote 3, `fill`, `center`: 27 px are cut at the top and at the bottom.
- The same image with `fit`: it is scaled to 450 x 800, with a 15 px bar on each side.
- The same image with `none`, `top`: it is cut at 800 px.
- A 480 x 800 image on the Remote Two, `fill`: it is scaled to 512 x 854 and 16 px are cut at each
  side.
- The same image with `none`: a 54 px black strip remains, at the bottom with `top` or split with
  `center`.

### D4 — A user-set black scrim keeps the buttons readable

A `Rectangle` in the `bg` token (black) over the image, with `opacity = dim / 100`. `dim` is 0–80 and defaults to 40.
A missing or non-numeric value gives 40; a value outside the range is clamped.

- Rejected: a fixed scrim like the main page's 70 %. It needlessly darkens dark images.
- Rejected: an automatic scrim computed from the image lightness, as for media artwork. It is more
  code, and a decoded image can't be sampled cheaply from QML.
- Legibility (`platform-constraints` "Legibility"): the scrim is user-adjustable. The user sets
  `dim` from 0 to 80 % per page in the web-configurator settings dialog, and the core accepts that
  range. A default of 40 % keeps the off-white buttons readable on typical photos without hiding
  the picture, and a user who needs more contrast raises it. The default was accepted by the
  maintainer on 2026-11-23, so it is not an open question.

### D5 — One full-screen layer that cross-fades, not an image per page delegate

A new component `UiPageBackground.qml` is the first child of the `Activity.qml` and `Remote.qml`
screens, so it lies below the title, the `PathView` and the `PageIndicator` and fills the whole
screen. It is bound to the background of `pages[uiPages.currentIndex]`.

- It holds two `Image`s. When the resolved background changes, the new values go to the hidden
  `Image`. Once that image reports `Image.Ready` (or `Null` when there's no image), it fades in
  over 200 ms while the other fades out.
- When the resolved background is equal (same file, scale, align and dim), nothing happens. A
  shared image therefore doesn't flicker when swiping.
- A `user_interface` update re-assigns `pages` and re-evaluates the binding, so a changed
  background fades in.

Rejected: an `Image` in each `PathView` delegate, sliding with the finger. To cover the title, the
image would need a negative offset and z-order changes; the same image on every page would visibly
slide; and with `cacheItemCount: 5`, up to 7 full-screen images would be decoded at once.

### D6 — `sourceSize` is not set on the UI page background

The allowed images are at most 480 x 854 (D1), so decoding them at full size is cheap. A
`sourceSize` would also make Qt scale the image in `none` mode, so the `Image`s leave it unset.
They load with `asynchronous: true` and `cache: true`.

### D7 — The normalisation is in C++, the mapping in QML

`Resources` gets `Q_INVOKABLE QVariantMap getPageBackground(const QVariant& background)`. It
returns `{source, scale, align, dim}`:

- `source` is resolved through the existing `getResource(BackgroundImage, image)`, so an empty,
  malformed or missing image gives `""`.
- `scale` is `"fill"`, `"fit"` or `"none"`; unknown or missing values give `"fill"`.
- `align` is `"top"`, `"center"` or `"bottom"`; unknown or missing values give `"center"`.
- `dim` is a `qreal` from 0.0 to 0.8; missing or non-numeric values give 0.4.

`UiPageBackground.qml` only maps the three `scale` tokens to a `fillMode` and the three `align`
tokens to a `verticalAlignment`.

- Rejected: a `Q_GADGET` return value. It needs metatype registration, and QML could not use the
  Qt Quick `Image` enum values from C++ without the private header.
- Rejected: parsing in QML, which ADR 0012 rules out and which couldn't be unit tested.

### D8 — Main page images: a 480 x 275 image renders exactly as today, a taller one is cut to its top 480 x 275 part

The header `Image` in `Page.qml` keeps its setup for a 480 x 275 image: centre-aligned
`PreserveAspectCrop`, with `sourceSize` bound to the header size. For an image taller than the
480 : 275 aspect ratio (480 x 800 or 480 x 854), the image is cut to its top 480 x 275 region and
then rendered exactly like a 480 x 275 image.

- `Resources` gets `Q_INVOKABLE QRect getBackgroundImageClip(const QString& id)`. It resolves the
  file like `getBackgroundImage()` and reads only the image header with `QImageReader::size()`;
  the image is not decoded. Like `getIconSize()`, it caches the size per file by modification
  time and file size, so a header binding that is evaluated again costs a hash lookup and a
  `stat`. An empty id returns an empty rect without calling `getResource()`, so a page without an
  image logs no "missing ':'" warning. When the image is taller than the 480 : 275 ratio, it returns the top
  region `QRect(0, 0, w, round(w * 275 / 480))`, which is `(0, 0, 480, 275)` for the allowed
  sizes. Otherwise, and for a missing or unreadable file, it returns an empty `QRect`.
- `Page.qml` keeps `sourceSize` bound to the header size and leaves `sourceClipRect` unset when
  the rect is empty, as today. For a tall image it decodes the image at the scale the header gives
  a 480 x 275 image, `s = max(headerWidth / 480, headerHeight / 275)`: `sourceSize` is
  `(round(480 * s), 0)` (height proportional) and `sourceClipRect` is
  `(0, 0, round(480 * s), round(275 * s))`. Both properties have a RESET in Qt 5.15
  (`qquickimage_p.h:71`, `qquickimagebase_p.h:70`), so `undefined` restores the full image.
  `sourceClipRect` needs `QtQuick 2.15`, which `Page.qml` already imports.
- In Qt 5.15 the clip rect is given in `sourceSize`-scaled coordinates, hence the scaled rect.
- The scale matters because `sourceSize` with `PreserveAspectCrop` scales a bitmap up as well as
  down: measured with Qt 5.15.19, a 480 x 275 file in the 680 px header is decoded at 1187 x 680.
  A clip without `sourceSize` would leave the tall image at 480 x 275 and the GPU would scale it,
  which looks softer.
- Result, measured on a desktop build with a smooth test image whose top 480 x 275 part equals a
  480 x 275 file: in the 260 px header the pixels are identical; in the 680 px header they differ
  by at most 6 / 255 per channel (the scaled source has a slightly different sampling grid), plus
  the bottom few rows, which sample the rows below the cut-out and lie under the header's black
  gradient. Same fill mode, scrim, gradient and mask.
- The header-only size read is a few hundred bytes of file I/O on the GUI thread, once per file
  and change, and is negligible next to the asynchronous decode.

Rejected: top-aligning the crop (`verticalAlignment: Image.AlignTop`), as first planned. It would
have shown more of a tall image when the header grows, but it moved every existing 480 x 275 image
by about 7 px: its bottom 15 px would be cut instead of 7–8 px at the top and the bottom. Main
pages must look exactly as before.

Rejected: always setting the clip rect, also for a 480 x 275 image. It would change the code path
of every existing image, which the maintainer ruled out.

Rejected: clipping before scaling, which would make the tall image pixel-identical. Qt Quick only
offers the clip after scaling; clipping first needs an image provider in C++ for a difference of a
few levels per channel.

### D9 — No model-specific code, and a missing file shows black

An image made for the other model, for example after a backup restore, is shown by its scale mode
like any other image. A background whose file is gone (deleted in the web-configurator) resolves
to `""` and the page is black, as a main page header is today.

### D10 — The page background scrim is added to the design system

The user-set scrim is a new use of opacity that design system principle 4 doesn't list. The same
change amends `docs/design-system.md`:

- Principle 4: opacity is also used for the scrim over a page background, at the strength the
  user sets per page (0–0.8, default 0.4).
- Section 7, "Components": add `Components.UiPageBackground`.

The component uses only tokens (`bg`), so `./design-check.sh` passes without an allow-list entry.

- Rejected: reusing `Components.Dim`. Its 0.85 is fixed and meant for sheets; a background under
  0.85 black would barely be visible.

### D11 — The minus and plus buttons of a number item have no fill at rest

On a page with a background, the `surface` boxes of the number item's minus and plus buttons
(`NumberWidget.qml:165-170`) cover the image, while the icon buttons of the grid
(`ButtonIcon.qml`) and the thin slider are transparent. The buttons lose their fill at rest and
keep the pressed state of section 6 of the design system: `textPrimary` fill, glyph in `bg`. The
design system's slider section gets the rule for number items next to the slider buttons.

- Rejected: keeping the `surface` fill. It hides the background behind every number item and is
  the only filled step button on a page whose other buttons are transparent.
- Not changed: the round `surfaceRaised` buttons of the slider item's `buttons` style, which the
  design system specifies, and the minus and plus buttons of the number entity screen, which has
  no page background.
- Trade-off: without a value, "−  —  +" reads as three similar glyphs; the em dash stays the
  design system's placeholder.

## Risks / Trade-offs

Failure Mode Analysis (Core-API event handling and qrc registration are touched):

- [`UiPageBackground.qml` missing from `main.qrc` fails only at runtime] → An explicit task
  checkbox, and the desktop walk opens an activity and a remote.
- [A `user_interface` update removes the current page, leaving `currentIndex` past the end] → The
  binding guards an `undefined` page and resolves it to "no background", so the screen stays
  black and no error is raised.
- [Two page changes in quick succession, where the second image finishes loading first] → Only
  the latest resolved background is applied; a stale `Ready` from an earlier source is ignored by
  comparing it with the current target.
- [The background file is deleted while it is shown] → The decoded texture stays until the page
  changes; the next resolution gives `""`. This is acceptable, and it matches the main page
  header.
- [A bright image makes the transparent buttons unreadable] → `dim` defaults to 40 %, and the
  web-configurator preview shows the result before saving.
- [A newer core sends unknown `scale` / `align` values, or a `dim` out of range] → They are
  normalised to the defaults or clamped (ADR 0018). Unit tests cover this.
- [The activity is in Error or Timeout] → The red title is opaque and stays readable over the
  image. The unavailable overlay (`z: 2000`) and the opaque `DropDownMenu` still cover the
  background.
- [The size probe cannot read the file (missing, unknown format, read error)] → It returns an
  empty rect, so the unchanged 480 x 275 path is used. A missing file already resolves to no
  image.
- [A future core allows a size other than the three] → A taller ratio is clipped to the top region
  at the image's own width; a wider or equal ratio takes the unchanged path.
- [The page with the web view widget has a background] → The widget is a scene-graph item drawn
  over the layer like any other item. Its opaque placeholder and an opaque web page cover the
  background in the widget's cells; a page with a transparent background shows the picture
  through, which is expected. The widget's own loading (`pageShown`) is not touched.
- [The Remote Two is rotated] → `ui.width` / `ui.height` are the logical, rotated size, and the
  layer fills the screen item, so it needs no rotation of its own.

Resource impact:

- **Memory:** at most two decoded backgrounds per screen, each at most 480 x 854 x 4 bytes, about
  1.6 MB, plus the same as GPU textures. The Qt pixmap cache keeps unreferenced images within its
  small default limit. That is under 7 MB in the worst case, against the 512 MB target.
- **CPU:** decoding is asynchronous, on Qt's image reader thread, once per change of image; a
  JPEG of this size decodes in tens of milliseconds. There are no timers and no polling.
- **Frame rate:** one full-screen textured quad and one full-screen blended scrim, plus a second
  pair only during the 200 ms fade. This is to be measured while swiping pages on both devices.
- **Battery:** idle work is unchanged. The fade runs only on a page change, never while idle or
  with the display off.
- **Main page size probe:** one header read per header binding, with no decode and no memory kept.
- **Memory next to the web view:** the web view widget has its own page memory. The background
  adds at most about 3.3 MB, which doesn't change the web view's budget.
- **Binary size and latency:** one small QML file. Input-to-command latency is unaffected.

## Migration Plan

- Order: core-api, then remote-core (and a core-simulator build containing it), then this change.
  The web-configurator change runs in parallel and is needed to set a background without
  hand-written REST calls.
- No data migration in the UI. Pages without `background` look as before; main pages with a
  480 x 275 image look exactly as before (D8).
- Verification target:
  - The desktop simulator (`UC_MODEL=DEV`, 480 x 850) against a core-simulator with the remote-core
    change, with backgrounds set over REST: rendering, scale modes, align, dim, the cross-fade, a
    `user_interface` update, removal with `background: null`, a missing file, a page with a web
    view widget (where the web engine is available), a main page with a 480 x 275 image
    (unchanged, compared with a screenshot of the base build) and one with a 480 x 800 image (its
    top 480 x 275 part).
  - Remote Two and Remote 3 devices: frame rate while swiping and resident memory, because the
    frame rate and GPU cost can't be judged on a desktop.
  - The d-pad page switch (DPAD_LEFT / DPAD_RIGHT) is walked with the keypad on a device, because
    it triggers the same cross-fade as a swipe.

## Open Questions

None.
