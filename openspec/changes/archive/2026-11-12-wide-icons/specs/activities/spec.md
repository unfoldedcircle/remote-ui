## ADDED Requirements

### Requirement: Icon size on activity page buttons
An image icon on a text or icon button of an activity page SHALL use the whole button as its box: it SHALL keep its own aspect ratio, be drawn as large as the button allows without being cut or stretched, and be centred. A glyph icon and the placeholder of a failed icon SHALL be drawn at 40 % of the button's shorter side, centred. The examples assume a Remote 3 with a 4 × 6 grid, where one cell is 120 x 120 px.

#### Scenario: Square icon on a 1 × 1 button
- **WHEN** a 1:1 SVG icon is placed on a 1 × 1 button
- **THEN** it is drawn 120 x 120, filling the button

#### Scenario: Square icon on a 2 × 1 button
- **WHEN** a 1:1 SVG icon is placed on a 2 × 1 button
- **THEN** it stays square at 120 x 120, centred, with empty space left and right

#### Scenario: Wide icon on a matching button
- **WHEN** a 2:1 SVG icon is placed on a 2 × 1 button
- **THEN** it is drawn 240 x 120 and fills the button

#### Scenario: Very wide icon on a 4 × 1 button
- **WHEN** a 4:1 SVG icon is placed on a 4 × 1 button
- **THEN** it is drawn 480 x 120 and fills the button

#### Scenario: Very wide icon on a taller button
- **WHEN** a 4:1 SVG icon is placed on a 4 × 2 button
- **THEN** it is drawn 480 x 120, vertically centred

#### Scenario: Wide icon on a 1 × 1 button
- **WHEN** a 2:1 SVG icon is placed on a 1 × 1 button
- **THEN** it fits the width at 120 x 60, vertically centred

#### Scenario: Tall icon on a wide button
- **WHEN** a 1:2 SVG icon is placed on a 2 × 1 button
- **THEN** it is drawn 60 x 120, horizontally centred

#### Scenario: Tall icon on a tall button
- **WHEN** a 1:4 SVG icon is placed on a 1 × 4 button
- **THEN** it is drawn 120 x 480 and fills the button

#### Scenario: Wide icon on a whole-page button
- **WHEN** a 4:1 SVG icon is placed on a button covering the whole 4 × 6 page (480 x 720)
- **THEN** it is drawn 480 x 120, centred on the page, and is not rasterized larger than that

#### Scenario: Glyph icon unchanged
- **WHEN** `uc:power-on` is placed on a 2 × 1 button
- **THEN** the glyph is drawn at 48 px, centred, as before this change

#### Scenario: Bitmap icon unchanged
- **WHEN** a PNG custom icon is placed on a 2 × 1 button
- **THEN** it is drawn as before this change

#### Scenario: Icon file that cannot be used
- **WHEN** a button's SVG icon is missing, rejected or fails to decode
- **THEN** a missing file leaves the button without an icon, and a rejected or undecodable file shows the placeholder glyph at 40 % of the shorter side

#### Scenario: Icon still loading
- **WHEN** a page is shown while its SVG icons are still being decoded
- **THEN** each button shows no icon and no placeholder until its icon is ready, and it already reacts to a tap
