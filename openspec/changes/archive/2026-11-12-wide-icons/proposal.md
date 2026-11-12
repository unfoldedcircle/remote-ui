## Why

remote-core now accepts SVG custom icons and TV channel icons that are not square: the root
`viewBox` may have an aspect ratio from 1:4 to 4:1, inclusive. PNG and JPEG icons are unchanged.
On activity and remote pages the user places icon buttons of any size in the page grid, from one
cell up to the whole page. A wide button is where a wide icon belongs: a 2:1 logo on a 2 × 1 button
should use the whole button. A square icon on the same button should stay square and should not be
stretched. In short, the UI should show what the user expects.

The UI already lays out image icons on page buttons this way, but nothing specifies it and nothing
tests it, so a later change could break it without anyone noticing. Under the layout, a non-square
SVG is rasterized larger than it is drawn. Qt 5.15 decodes an image with `PreserveAspectFit` so
that it *covers* the requested box, and only then scales it down to fit (design.md, Context). A 1:4
icon on a 4 × 1 button is rasterized at 480 × 1920 px and drawn at 30 × 120 px. Now that the core
accepts wide icons, this case becomes common. It wastes memory, and the large downscale costs
sharpness.

## What Changes

- An SVG icon on an activity or remote page button keeps its own aspect ratio and is drawn as large
  as the button allows, centred: a square icon on a 2 × 1 button stays square, a 2:1 icon on a 2 × 1
  button fills it, and a 2:1 icon on a 1 × 1 button fits the width. This matches what the UI
  already does today, and is now specified and tested.
- An SVG icon is rasterized at the size it is drawn, not at the larger size that covers its box.
  This saves memory and keeps the icon sharp. It applies wherever an SVG icon is shown; every
  other icon box keeps its square shape and looks the same.
- The aspect ratio of an SVG icon is the ratio of its root `viewBox`. A root `width` and `height`
  that declare another ratio no longer squash the icon: a 2:1 flag declared `width="100px"
  height="100px"` is drawn 2:1, not as a square.
- Glyph icons (`uc:`), PNG and JPEG icons, the placeholder of a failed icon and every square icon
  box elsewhere in the UI look the same as before.

## Non-Goals

- Larger glyph (`uc:`) icons on page buttons. They are drawn too small today; that is a future
  enhancement.
- Padding, an icon together with text on one item (not allowed today), pressed feedback and
  tinting of SVG icons.
- A wider box for any other screen that shows a user-chosen icon: tiles, titles, the icon selector,
  lists and widgets keep their square box.
- A UI-side check or clamp of the SVG aspect ratio, and any change for PNG or JPEG icons.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `ui-resources`: a new requirement for the raster size of an SVG icon whose aspect ratio differs
  from its box.
- `activities`: a new requirement for how an icon is sized on an activity page button.
- `remote-entity`: a new requirement for how an icon is sized on a remote page button.

## Impact

- **Hardware models:** both. No hardware-specific behaviour.
- **remote-core / Core-API dependency:** the SVG icon upload with an aspect ratio of 1:4 to 4:1,
  which the core already provides through the `Icon` and `TvChannelIcon` resources. No new
  Core-API message or field. UI and core ship in the same firmware (ADR 0005).
- **Third-party libraries or assets:** none. The Qt SVG image plugin is already linked.
- **Code (planned, see tasks.md):** `src/ui/resources.{h,cpp}`, `src/ui/imageDecoder.{h,cpp}`,
  `src/qml/components/Icon.qml`, `test/ui/test_icon_files.cpp`, `test/ui/test_image_decoder.cpp`,
  `CHANGELOG.md`. No new files, so there is nothing to register.
- **Builds on:** the archived change `svg-custom-icons`, whose requirements are in the living
  `ui-resources` spec. This change only adds requirements and modifies none of them.

## Follow-ups outside this change

None. An earlier draft asked remote-core and the web configurator to reject SVG icons whose root
width/height ratio differs from the viewBox ratio. That is not needed: the UI takes the ratio from
the viewBox (design.md, D9), so such a file is drawn undistorted.
