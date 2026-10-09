## MODIFIED Requirements

### Requirement: Entity command request
Every command to an entity SHALL be sent as an `execute_entity_command` request with `entity_id`, `cmd_id` and, only when it is not empty, `params`. Commands issued by the UI's own entity controls SHALL use `<entity type>.<command>` in lower case (e.g. `light.toggle`, `media_player.play_pause`, `cover.stop`); voice assistant commands SHALL be sent without a type prefix (`voice_start`). A `cmd_id` and `params` taken from the configuration (button mappings, UI pages, sequences) SHALL be sent exactly as configured, never renamed, re-cased or completed; the only exception is the wrapping of a simple IR command into `remote.send` described in the `remote-entity` capability. The UI's local command key used for tracking SHALL never be sent to the core.

#### Scenario: Switch toggled from its tile
- **WHEN** the user taps the icon of the tile of a switch with feature `toggle`
- **THEN** `execute_entity_command` is sent with `cmd_id` `switch.toggle` and no `params`

#### Scenario: Configured command with mixed case
- **WHEN** a button mapping has `cmd_id` `remote.send_cmd` with params `{command: "Menu"}`
- **THEN** exactly that `cmd_id` and those params are sent
