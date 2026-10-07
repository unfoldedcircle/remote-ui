## MODIFIED Requirements

### Requirement: Activity menu and included entities
The activity menu SHALL list a "Fix states" row followed by "Quickly access entities included in this activity:" and the activity's included entities except macros. Tapping an entity SHALL open its control screen on top of the activity. DPAD_UP/DOWN SHALL move over the top row and the entities, DPAD_MIDDLE SHALL open the selected entry, BACK SHALL return from the "Fix states" page to the menu and close the menu from there. The selected entry SHALL be drawn in the main UI's fill style and only while the keypad is active (see `key-navigation`).

#### Scenario: Open an included entity
- **WHEN** the user taps an included media player
- **THEN** the media player screen opens above the activity screen and takes the physical buttons until it closes

#### Scenario: Menu opened by touch
- **WHEN** the user opens the activity menu by tapping the title
- **THEN** no entry is marked until the first d-pad press; the selection then appears where that press moved it

#### Scenario: Menu opened while the keypad is active
- **WHEN** the activity menu opens while the keypad is active
- **THEN** the "Fix states" row is filled with the selection fill right away
