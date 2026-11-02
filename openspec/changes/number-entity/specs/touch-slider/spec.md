## MODIFIED Requirements

### Requirement: Slider feature per entity
The slider SHALL control: for a media player, "volume" (requires the Volume feature) or "seek" (requires Seek, Media_duration and Media_position); for a light, "dim" or "brightness" (requires Dim); for a cover, "position" (requires Position); for a number entity, "value" (requires `min` and `max`). A feature of "default" or none SHALL resolve by entity type to volume, dim, position or value respectively. If the entity's features are not loaded yet the UI SHALL fetch the entity once and re-evaluate when its features arrive; a number entity's value needs no feature. An unsupported entity type, feature or missing capability SHALL leave the slider inactive.

#### Scenario: Activity slider without explicit feature
- **WHEN** an activity maps the slider to a light with feature "default"
- **THEN** the slider dims the light

#### Scenario: Media player without volume feature
- **WHEN** the slider is mapped to a media player that lacks the Volume feature
- **THEN** the slider stays inactive

#### Scenario: Unbounded number
- **WHEN** an activity maps the slider to a number entity with `max` null
- **THEN** the slider stays inactive

## ADDED Requirements

### Requirement: Number entity on the touch slider
For a number entity the touch slider SHALL map a full sweep to the gain times the range `max - min`, move the target in whole steps of `step` within the range, and show the entity name and the value formatted like the number entity with its unit. A number without a value SHALL start the gesture at `min`. The target SHALL be sent with `number.set_value` every 200 ms while it differs from the entity value, and once more on release.

#### Scenario: Swipe on a bass level
- **WHEN** the touch slider controls a number entity from -10 to 10 dB with step 0.5 and the user swipes
- **THEN** the overlay shows values like "2.5 dB" and `number.set_value` is sent with a multiple of 0.5
