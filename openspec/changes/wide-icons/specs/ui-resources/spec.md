## ADDED Requirements

### Requirement: SVG icon raster size
An SVG icon SHALL be drawn as the largest rectangle with its own aspect ratio that fits its box, centred, and SHALL be rasterized at that drawn size, never at a size that covers the box. Its aspect ratio SHALL be the one Qt reports for the file. When that size cannot be read, the icon SHALL be decoded at the box size as before. PNG and JPEG icons and glyph icons SHALL keep their current rendering.

#### Scenario: Wide SVG in a square box
- **WHEN** a 4:1 custom SVG icon is shown in a 100 px box
- **THEN** it is drawn 100 x 25, centred, and no larger raster than 100 x 25 is decoded

#### Scenario: Square SVG in a wide box
- **WHEN** a 1:1 custom SVG icon is shown in a 240 x 120 px box
- **THEN** it is drawn and rasterized at 120 x 120, centred, not at 240 x 240

#### Scenario: Size taken from root width and height
- **WHEN** an SVG has a `viewBox` of 0 0 200 100 and a root width and height of 20mm and 10mm
- **THEN** its aspect ratio is 2:1

#### Scenario: Size taken from the viewBox
- **WHEN** an SVG has a `viewBox` of 0 0 200 100 and a root width and height in `em`, in `%`, set to `auto` or missing
- **THEN** its aspect ratio is 2:1

#### Scenario: File replaced under the same name
- **WHEN** a 1:1 `custom:logo.svg` is replaced by a 2:1 file of the same name and the icon is shown again
- **THEN** it is drawn at 2:1

#### Scenario: Size cannot be read
- **WHEN** an SVG icon file exists but its size cannot be read
- **THEN** it is decoded at the box size, and if decoding fails the placeholder icon is shown

#### Scenario: Bitmap icon unchanged
- **WHEN** a PNG custom icon is shown in a 240 x 120 px box
- **THEN** it is decoded and drawn exactly as before this change
