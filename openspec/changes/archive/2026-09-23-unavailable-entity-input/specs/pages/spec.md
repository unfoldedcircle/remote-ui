## MODIFIED Requirements

### Requirement: Tile selection by d-pad
DPAD_UP / DPAD_DOWN SHALL move the selection through the tiles of the current page; inside an open group they walk the group's entity rows before leaving the group. DPAD_MIDDLE SHALL trigger the selected tile's quick action (toggle) and a long press SHALL open its control screen; with the "Inverted button behaviour" setting both are swapped. CHANNEL_UP closes and CHANNEL_DOWN opens the selected group. A tile whose entity is Unavailable SHALL refuse both presses and report the refusal, on a page and inside an open group alike (see `entity-commands`).

#### Scenario: Group row that is off
- **WHEN** DPAD_MIDDLE is pressed on a row of an open group whose entity is unavailable (state 0)
- **THEN** no command is sent and the notification "<name> is unavailable" is shown
