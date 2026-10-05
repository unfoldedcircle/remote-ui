## MODIFIED Requirements

### Requirement: No commands to an unavailable entity from its controls
While an entity is Unavailable, every control that would send it a command SHALL refuse the command instead of sending it, and SHALL show the warning notification "<name> is unavailable" naming the entity. The refusal SHALL be the same wherever the command is triggered: the quick action of an entity tile and the opening of its control screen, by touch and with DPAD_MIDDLE, short press and long press, for a tile placed directly on a page and for the same tile inside an open group alike; every key of the entity's control screen that sends a command, the light brightness and colour temperature keys included; and a button mapping of an activity that addresses the entity. On the control screen the "Entity unavailable" overlay is the feedback and no extra notification SHALL be raised. A tile whose entity could not be loaded at all SHALL refuse the command without a notification, because there is no name to report. BACK and HOME SHALL always leave an entity control screen and the long press that opens the entity edit menu SHALL stay available (see `entity-detail-controls`). Commands the UI issues outside these controls SHALL still be sent and fail through the core, and a command issued while the remote is waking up SHALL be sent (see "Commands are accepted while the remote is waking up").

#### Scenario: Control screen of an unavailable light
- **WHEN** the light becomes Unavailable while its control screen is open and the user presses DPAD_UP and then BACK
- **THEN** nothing is sent, no notification is raised, and BACK closes the screen

#### Scenario: Unavailable tile on a page selected with the d-pad
- **WHEN** an unavailable switch tile with a quick action, placed directly on a page, is selected and DPAD_MIDDLE is pressed
- **THEN** no command is sent and the notification "<name> is unavailable" is shown

#### Scenario: Activity mapping to an unavailable device
- **WHEN** an available activity maps VOLUME_UP to a receiver that is Unavailable and the remote is awake
- **THEN** no command is sent and the notification names the receiver

#### Scenario: The same tile inside a group
- **WHEN** DPAD_MIDDLE is pressed on the row of an unavailable entity inside an open group
- **THEN** the behaviour is the same as for the tile on a page: no command and the "<name> is unavailable" notification

#### Scenario: Entity that could not be loaded
- **WHEN** DPAD_MIDDLE is pressed on a tile whose entity is not known to the UI
- **THEN** no command is sent and no notification is shown

## ADDED Requirements

### Requirement: Commands are accepted while the remote is waking up
Losing the connection to the core, a suspend of the remote included, marks every entity Unavailable until the entity list has been reloaded, so the UI does not know an entity's real availability while the remote is coming back. A command to an Unavailable entity SHALL therefore be accepted, and no "is unavailable" notification shown, while a resume is pending: from the moment the remote goes to sleep until the command retry window after a wakeup has closed (see `power-and-battery`). The command SHALL then follow the wakeup retry policy. When that window is configured as 0 s ("Disabled") no resume is pending, and an Unavailable entity SHALL be refused as at any other time. An available entity SHALL be commanded in both states.

#### Scenario: Button pressed right after a wakeup
- **WHEN** the user presses a key that commands an entity while the remote is waking up and every entity is still marked Unavailable
- **THEN** the command is sent and retried within the window, and no "is unavailable" notification appears

#### Scenario: Unavailable entity on an awake remote
- **WHEN** the same key is pressed for an Unavailable entity while no resume is pending
- **THEN** the command is refused and the "<name> is unavailable" notification is shown

#### Scenario: Available entity while a resume is pending
- **WHEN** a command is issued for an available entity while the remote is coming back from a wakeup
- **THEN** the command is sent

#### Scenario: Retry after wakeup disabled
- **WHEN** "Retry commands after wakeup" is set to 0 s and a key is pressed for an Unavailable entity
- **THEN** the command is refused with the notification, as on an awake remote
