## MODIFIED Requirements

### Requirement: Device class fallback
An entity's type and device class SHALL select its control screen through one embedded registry, the single place where a screen is registered: per type a screen directory and a default screen, per device class its screen, and the aliases. A missing or unknown device class SHALL fall back to the type's default: button `button`, switch `switch`, climate `climate`, cover `blind`, light `light`, media player `speaker`, remote `remote`, sensor `custom`, activity `activity`, macro `macro`, select `select`. Covers with device class `shade` SHALL be shown as `blind`, and `door` and `gate` as `window`. A type the registry has no screen for — the voice assistant, an unsupported type — SHALL resolve to no screen. A unit test SHALL verify that every type and device class resolves as listed here and that every screen the registry can name is embedded in the binary's resources.

#### Scenario: Unknown media player class
- **WHEN** a media player reports device class `projector`
- **THEN** it is handled as a `speaker`

#### Scenario: Gate cover
- **WHEN** a cover reports device class `gate`
- **THEN** its control screen is the window cover screen

#### Scenario: Type without a screen
- **WHEN** the registry is asked for the screen of a voice assistant or of an unsupported entity type
- **THEN** it answers with no screen

#### Scenario: Screen not embedded
- **WHEN** a screen is added to the registry but not to the resource file
- **THEN** the unit test fails, before any build reaches a device
