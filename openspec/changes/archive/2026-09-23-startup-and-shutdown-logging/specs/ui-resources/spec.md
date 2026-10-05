## MODIFIED Requirements

### Requirement: Missing and default icons
An icon that cannot be resolved SHALL leave its area empty and SHALL only be logged: an identifier without a colon, an unknown prefix, or a custom or TV channel icon whose file does not exist. An element with no icon identifier at all SHALL leave its area empty without asking for a lookup and without logging anything, because having no icon is a normal state. A `uc:` name that is not in the mapping SHALL fall back to a TV channel icon file of the same name and otherwise stay empty. An entity without an icon SHALL get a default icon by type: button and switch `uc:power-on`, climate `uc:climate`, cover `uc:blind`, light `uc:light`, media player `uc:music`, sensor `uc:sensor`, remote `uc:remote`, activity and macro `uc:activity`, voice assistant `uc:microphone`, select `uc:list-dropdown`, any other type `uc:warning`.

#### Scenario: Custom icon file deleted
- **WHEN** an entity uses `custom:old.png` and the file no longer exists
- **THEN** the tile shows no icon and no placeholder

#### Scenario: Unknown built-in name
- **WHEN** an entity uses `uc:does-not-exist`
- **THEN** the icon is empty unless `<UC_RESOURCE_PATH>/TvChannelIcon/does-not-exist` exists, in which case that file is drawn

#### Scenario: Entity without icon
- **WHEN** the core delivers a light entity without an icon
- **THEN** the light shows `uc:light`

#### Scenario: Element with no icon identifier
- **WHEN** a screen element is shown whose icon identifier is empty
- **THEN** its icon area stays empty and nothing is logged
