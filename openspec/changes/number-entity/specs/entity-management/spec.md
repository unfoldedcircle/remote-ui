## MODIFIED Requirements

### Requirement: Supported entity types and default icons
The UI SHALL support the entity types `activity`, `button`, `climate`, `cover`, `light`, `macro`, `media_player`, `number`, `remote`, `select`, `sensor`, `switch` and `voice_assistant`. An entity without an icon SHALL get a default icon per type: button and switch `uc:power-on`, climate `uc:climate`, cover `uc:blind`, light `uc:light`, media player `uc:music`, number `uc:input-numeric`, sensor `uc:sensor`, remote `uc:remote`, activity and macro `uc:activity`, voice assistant `uc:microphone`, select `uc:list-dropdown`. An entity of any other type SHALL still be kept as an unsupported entity: it shows its name and its icon (default `uc:warning`), has no state, no features and no attributes, and offers no quick action.

#### Scenario: Light without icon
- **WHEN** the core delivers a light without an `icon`
- **THEN** the light is shown with the `uc:light` icon

#### Scenario: Number without icon
- **WHEN** the core delivers a number entity without an `icon`
- **THEN** the number entity is shown with the `uc:input-numeric` icon

#### Scenario: Unknown entity type
- **WHEN** the core delivers an entity of type `water_heater`
- **THEN** the entity is kept and shown with its name and the `uc:warning` icon, without a state and without a quick action

### Requirement: Device class fallback
An entity's type and device class SHALL select its control screen through one embedded registry, the single place where a screen is registered: per type a screen directory and a default screen, per device class its screen, and the aliases. A missing or unknown device class SHALL fall back to the type's default: button `button`, switch `switch`, climate `climate`, cover `blind`, light `light`, media player `speaker`, number `number`, remote `remote`, sensor `custom`, activity `activity`, macro `macro`, select `select`. Covers with device class `shade` SHALL be shown as `blind`, and `door` and `gate` as `window`. Every number device class SHALL be shown as `number`. A type the registry has no screen for — the voice assistant, an unsupported type — SHALL resolve to no screen. A unit test SHALL verify that every type and device class resolves as listed here and that every screen the registry can name is embedded in the binary's resources.

#### Scenario: Unknown media player class
- **WHEN** a media player reports device class `projector`
- **THEN** it is handled as a `speaker`

#### Scenario: Gate cover
- **WHEN** a cover reports device class `gate`
- **THEN** its control screen is the window cover screen

#### Scenario: Number with a device class
- **WHEN** a number entity reports device class `temperature`
- **THEN** its control screen is the number screen

#### Scenario: Type without a screen
- **WHEN** the registry is asked for the screen of a voice assistant or of an unsupported entity type
- **THEN** it answers with no screen

#### Scenario: Screen not embedded
- **WHEN** a screen is added to the registry but not to the resource file
- **THEN** the unit test fails, before any build reaches a device

## ADDED Requirements

### Requirement: Entity option updates
An entity change event that carries `options` SHALL apply them to the entity, also when the options object is empty, so that removing the last user override in `options.user` takes effect without reloading the entity.

#### Scenario: Last override removed
- **WHEN** a sensor without driver options has the user override `decimals` 1 and an entity change event delivers empty options
- **THEN** the sensor value is shown in the automatic format again

### Requirement: User overrides of entity options
The UI SHALL apply the user overrides in an entity's `options.user` wherever the entity is shown, taking precedence over the driver option of the same name: `decimals` for sensor and number entities, `display_mode` for number entities. An override with an invalid value SHALL be ignored. The UI SHALL NOT edit the overrides.

#### Scenario: Override wins
- **WHEN** a number entity has the option `decimals` 2 and the user override `decimals` 0
- **THEN** its value is shown without decimal places on its tile, control screen and widgets
