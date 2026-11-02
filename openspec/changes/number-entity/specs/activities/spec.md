## MODIFIED Requirements

### Requirement: Activity UI pages
The activity screen SHALL render the pages of the activity's `user_interface` option as swipeable pages with a page indicator when there is more than one. Each page SHALL be a grid of `grid.width` × `grid.height` cells (default 4 × 6); an item occupies `location` and `size` (default 1 × 1). Item types text, icon, media_player, sensor, select, slider and number SHALL be rendered; other types are ignored. An empty page SHALL read "Empty page" and "You can add UI elements via the Web Configurator". DPAD_LEFT/DPAD_RIGHT SHALL switch pages. A `user_interface` update from the core SHALL re-render the pages.

#### Scenario: Text button
- **WHEN** the user taps a text or icon item with a command
- **THEN** the item's `cmd_id` with its `params` is sent to the item's entity

#### Scenario: Macro item
- **WHEN** the user taps an item whose entity is a macro
- **THEN** the progress overlay is shown for the macro run

#### Scenario: Start another activity from a page
- **WHEN** an item sends `activity.start` to another activity
- **THEN** the current activity screen closes and the other activity's screen opens after 1 s

#### Scenario: Slider item
- **WHEN** an activity page has a slider item bound to a light
- **THEN** the item is rendered as a slider for the light's brightness
