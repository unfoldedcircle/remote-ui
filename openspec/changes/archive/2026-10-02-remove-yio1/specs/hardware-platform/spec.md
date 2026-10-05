## MODIFIED Requirements

### Requirement: Hardware model selection
The UI SHALL select the hardware model from the environment variable `UC_MODEL`, compared case-insensitively against `DEV`, `UCR2` and `UCR3`. An unset or unrecognised value SHALL select `DEV`. The selected model SHALL be reported as the model number everywhere the UI shows or checks it; the model name reported by the core is ignored. The model of the first YIO remote is not supported and SHALL NOT exist as a value.

#### Scenario: Valid model
- **WHEN** the app starts with `UC_MODEL=ucr3`
- **THEN** the UI runs as Remote 3 and reports model number `UCR3`

#### Scenario: Invalid or missing model
- **WHEN** the app starts with `UC_MODEL` unset or set to an unknown value
- **THEN** the UI runs as the desktop simulator (`DEV`)

#### Scenario: Legacy YIO1 model
- **WHEN** the app starts with `UC_MODEL=YIO1`
- **THEN** the value is unrecognised and the UI runs as the desktop simulator (`DEV`), which is what `YIO1` behaved like

### Requirement: Regulatory information flag
The UI SHALL expose a regulatory-information flag that is true on Remote Two and Remote 3 and false on the desktop. The flag SHALL only decide whether the desktop button simulator window is created; the About section lists "Regulatory" on every model.

#### Scenario: Device
- **WHEN** the model is `UCR2` or `UCR3`
- **THEN** the flag is true and no button simulator window is created

#### Scenario: Desktop
- **WHEN** the model is `DEV`
- **THEN** the flag is false and the button simulator window is shown; the About section still lists "Regulatory"

### Requirement: Key navigation availability per model
The UI SHALL report keypad navigation as enabled for the models `UCR2`, `UCR3` and `DEV`, i.e. for every model. Key navigation SHALL be reported as active only while it is enabled and the keypad is currently in use (the last input came from a physical key rather than from touch).

#### Scenario: Keypad in use
- **WHEN** a physical key is pressed on any supported model
- **THEN** key navigation becomes active and keypad selection highlights are rendered

#### Scenario: Touch resumes
- **WHEN** the screen is touched
- **THEN** key navigation becomes inactive and the highlights disappear
