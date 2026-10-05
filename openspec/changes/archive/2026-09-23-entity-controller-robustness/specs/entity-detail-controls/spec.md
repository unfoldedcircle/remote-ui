## MODIFIED Requirements

### Requirement: Opening an entity control screen
Tapping an entity tile outside its icon SHALL open the entity's control screen when the entity is enabled and the page is not in edit mode. The screen SHALL be chosen by entity type and device class, slide up from the bottom in 300 ms, and request the entity from the core again when it opens so that it shows current data. While a control screen is already shown, another one SHALL NOT be opened on the same level. When the UI has no screen for the entity — a type it implements none for (a voice assistant, or an entity type only a newer core knows, which is kept as an unsupported entity) or a device class whose screen is missing — the screen SHALL close again by itself, with no empty screen left on display, and the failed screen SHALL be logged. The UI SHALL stay operable afterwards: the next entity, activity or settings page SHALL open normally, both for a screen opened from a page or a group and for one opened on top of an activity.

#### Scenario: Tap on a light tile
- **WHEN** the user taps the name area of an enabled light tile
- **THEN** the light control screen slides up
- **AND** the entity is fetched from the core and its attributes are updated from the response

#### Scenario: Entity not enabled
- **WHEN** the user taps the tile of an entity that is not enabled
- **THEN** no control screen opens

#### Scenario: Entity without a screen
- **WHEN** the user opens a voice assistant entity, or an entity of a type this UI does not implement
- **THEN** the screen closes again immediately, the page underneath stays usable and the missing screen is logged

#### Scenario: Another screen after a failed one
- **WHEN** an entity without a screen was opened and the user then opens a light, an activity or a settings page
- **THEN** that screen opens normally

#### Scenario: Entity without a screen from an activity
- **WHEN** an entity of the activity's device list has no screen and the user opens it
- **THEN** it closes again and the activity screen stays as it was, and further entities of the list still open
