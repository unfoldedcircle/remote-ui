## ADDED Requirements

### Requirement: Icon size on remote page buttons
An image icon on a button of a remote's custom UI page SHALL be sized exactly as on an activity page button: the whole button is its box, the icon keeps its own aspect ratio, is drawn as large as the button allows without being cut or stretched, and is centred. A glyph icon and the placeholder SHALL be drawn at 40 % of the button's shorter side.

#### Scenario: Wide icon on a matching button
- **WHEN** a 2:1 SVG icon is placed on a 2 × 1 button of a remote page on a Remote 3 with a 4 × 6 grid
- **THEN** it is drawn 240 x 120 and fills the button

#### Scenario: Square icon on a wide button
- **WHEN** a 1:1 SVG icon is placed on a 2 × 1 button of a remote page
- **THEN** it stays square at the button's height, centred
