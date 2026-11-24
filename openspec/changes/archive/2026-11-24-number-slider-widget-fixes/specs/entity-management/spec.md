## ADDED Requirements

### Requirement: Option updates for every entity type
An entity change event that carries `options` SHALL replace the options of a loaded switch, light, media player, climate and sensor entity, as it does for a number entity. An option missing from the delivered options SHALL have its default. Screens and widgets showing the entity SHALL follow without reloading it.

#### Scenario: Option removed
- **WHEN** an entity change event delivers options without `color_temperature_steps` for a light that had 3 steps
- **THEN** the light's colour temperature slider offers every value of 0 to 100 again

### Requirement: Options that follow an update
The options that follow an entity change event SHALL be: switch `readable`; light `color_temperature_steps`; media player `volume_steps` and `simple_commands`; climate `temperature_unit`, `fan_modes`, `min_temperature`, `max_temperature` and `target_temperature_step`; sensor `custom_label`, `custom_unit`, `native_unit`, `min_value`, `max_value` and `decimals`.

#### Scenario: Climate unit changed
- **WHEN** an entity change event delivers `temperature_unit` `FAHRENHEIT` for a climate entity shown in °C
- **THEN** its temperatures are labelled °F without reloading the entity

#### Scenario: Sensor unit changed
- **WHEN** an entity change event delivers `custom_unit` `kWh` for a sensor shown on an activity page
- **THEN** the sensor widget shows the value with kWh
