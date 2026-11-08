## ADDED Requirements

### Requirement: SVG icon raster size
An SVG icon SHALL be drawn as the largest rectangle with its own aspect ratio that fits its box, centred, and SHALL be rasterized at that drawn size, never at a size that covers the box. Its aspect ratio SHALL be the ratio of its root `viewBox`, whatever its root width and height declare. PNG and JPEG icons and glyph icons SHALL keep their current rendering.

#### Scenario: Wide SVG in a square box
- **WHEN** a 4:1 custom SVG icon is shown in a 100 px box
- **THEN** it is drawn 100 x 25, centred, and no larger raster than 100 x 25 is decoded

#### Scenario: Square SVG in a wide box
- **WHEN** a 1:1 custom SVG icon is shown in a 240 x 120 px box
- **THEN** it is drawn and rasterized at 120 x 120, centred, not at 240 x 240

#### Scenario: viewBox wider than the declared size
- **WHEN** an SVG has a `viewBox` of 0 0 200 100 and a root width and height of 100px each, and is shown on a 2 × 1 button of 240 x 120 px
- **THEN** it is drawn and rasterized at 240 x 120, with its content undistorted, not squashed into a square

#### Scenario: viewBox square, declared size wide
- **WHEN** an SVG has a `viewBox` of 0 0 100 100 and a root width and height of 200 and 100
- **THEN** its aspect ratio is 1:1 and it is drawn square, undistorted

#### Scenario: Root width and height in any unit
- **WHEN** an SVG has a `viewBox` of 0 0 200 100 and a root width and height in px, `mm`, `em`, `%`, set to `auto` or missing
- **THEN** its aspect ratio is 2:1

#### Scenario: No usable viewBox
- **WHEN** an SVG has no `viewBox`, or one that is not four numbers with a positive width and height, and a root width and height of 200 and 100
- **THEN** its aspect ratio is the 2:1 Qt reports for the file, and when Qt reports no size either, it is decoded at the box size

#### Scenario: File replaced under the same name
- **WHEN** a 1:1 `custom:logo.svg` is replaced by a 2:1 file of the same name and the icon is shown again
- **THEN** it is drawn at 2:1

#### Scenario: Size cannot be read
- **WHEN** an SVG icon file exists but its size cannot be read
- **THEN** it is decoded at the box size, and if decoding fails the placeholder icon is shown

#### Scenario: Bitmap icon unchanged
- **WHEN** a PNG custom icon is shown in a 240 x 120 px box
- **THEN** it is decoded and drawn exactly as before this change
