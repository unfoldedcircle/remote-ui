# touch-slider Specification

## Purpose

The Remote 3 touch slider: what it controls, gain settings, the default function per entity type, readiness checks before sending commands, and its overlay.

## Requirements

### Requirement: Touch slider availability
The touch slider SHALL only produce input on Remote 3, where it is read from the input device named by `UC_TOUCHSLIDER_DEV_PATH`. The slider overlay SHALL deactivate itself on any model other than `UCR3` or `DEV`. A global "Touch slider" switch under Settings (stored on the remote, default on, "When off, the touch slider is disabled everywhere and swiping it does nothing.") SHALL disable every slider overlay.

#### Scenario: Remote Two
- **WHEN** an entity with a slider feature is opened on `UCR2`
- **THEN** no slider overlay is armed

#### Scenario: Slider disabled
- **WHEN** the global switch is off
- **THEN** swiping the hardware slider does nothing anywhere; turning it back on re-arms the current screen

### Requirement: Slider input device handling
The driver SHALL open the input device non-blocking, read its axis range and resting position, and retry opening every 1 s while the device is missing or cannot be opened. Events SHALL be grouped into frames delimited by SYN_REPORT: a touch-down frame produces a press with the frame's position, further frames with a position while touching are coalesced so that one drain of the device queue yields at most one move (latest position), and a touch-up frame first flushes a pending move and then produces a release. After SYN_DROPPED all events up to and including the next SYN_REPORT SHALL be discarded. A read error or end-of-file SHALL close and reopen the device on the next event loop pass; the device is not reopened on resume from standby.

#### Scenario: Fast swipe
- **WHEN** many position events arrive between two event loop passes
- **THEN** only the last position is delivered as a single move

#### Scenario: Device disappears
- **WHEN** reading the device fails
- **THEN** the device is closed and reopened, retrying every 1 s until it works

### Requirement: Slider feature per entity
The slider SHALL control: for a media player, "volume" (requires the Volume feature) or "seek" (requires Seek, Media_duration and Media_position); for a light, "dim" or "brightness" (requires Dim); for a cover, "position" (requires Position). A feature of "default" or none SHALL resolve by entity type to volume, dim or position respectively. If the entity's features are not loaded yet the UI SHALL fetch the entity once and re-evaluate when its features arrive. An unsupported entity type, feature or missing capability SHALL leave the slider inactive.

#### Scenario: Activity slider without explicit feature
- **WHEN** an activity maps the slider to a light with feature "default"
- **THEN** the slider dims the light

#### Scenario: Media player without volume feature
- **WHEN** the slider is mapped to a media player that lacks the Volume feature
- **THEN** the slider stays inactive

### Requirement: Where the slider is armed
The slider SHALL be armed on media player, light and cover entity screens for that entity; on the home page for the currently visible activity or media-player item while no entity screen is open (an activity's target is its configured slider entity, or its first media player widget); and on an activity screen according to the activity's `touch_slider` option: enabled flag, entity id ("default" = the activity's media widget entity) and feature. A disabled activity slider configuration SHALL reset entity and feature to "default".

#### Scenario: Home page
- **WHEN** the home page shows an activity whose slider configuration targets a receiver
- **THEN** swiping the hardware slider changes the receiver's volume

#### Scenario: Entity screen on top
- **WHEN** an entity screen is opened over the home page
- **THEN** the home page slider is released and the entity screen's slider takes over

### Requirement: Gesture to value mapping
On press the UI SHALL compute value units per raw slider unit as range x gain / axis range (axis range from the driver, 300 if unknown), where the range is 100 for volume and cover position, 255 for light brightness and the media duration in seconds for seek. Finger movement SHALL be accumulated with sub-unit precision and applied in whole steps to a target value clamped to the range; each change SHALL play a Bump haptic at most once per 30 ms and update the overlay. While pressed the target SHALL be sent to the entity every 200 ms if it differs from the entity's value; on release it SHALL be sent once more if it still differs. External value changes SHALL update the overlay only while no gesture is active.

#### Scenario: Volume swipe
- **WHEN** the gain is 0.4 and the finger travels half the slider length on a media player at volume 20
- **THEN** the target becomes 40 and volume commands are sent every 200 ms during the swipe

#### Scenario: Clamped at end stop
- **WHEN** the target is already at 100 and the finger keeps moving up
- **THEN** no further haptic or command is produced

#### Scenario: Brightness display
- **WHEN** a light is dimmed with the slider
- **THEN** the overlay shows the brightness in percent while commands carry 0..255

### Requirement: Slider overlay
On press the UI SHALL open an overlay sliding up from the bottom over 300 ms with the entity name, a feature icon (volume, brightness, blind) or elapsed/remaining time for seek (formatted m:ss or h:mm:ss), a bar and the value. The overlay SHALL close 1 s after release, on tap outside, and on BACK or HOME. Other keys SHALL keep reaching the screen underneath.

#### Scenario: Release
- **WHEN** the finger is lifted
- **THEN** the overlay stays for 1 s and then slides down

#### Scenario: Volume key during a gesture
- **WHEN** VOLUME_UP is pressed while the slider overlay is open on a media player screen
- **THEN** the screen's own volume handler also runs, competing with the slider's target value

### Requirement: Sensitivity settings
The "Touch Slider" settings SHALL offer a gain per use case, "Volume" (default 0.4), "Brightness" (default 1.2), "Cover position" (default 1.2) and "Seek" (default 1.0), each from 0.1 to 2.0 in steps of 0.1 shown as "<x.x>x" ("1.0 means one full swipe covers the whole range"), stored on the remote and applied to the next gesture. Adjusting a gain SHALL play a Bump haptic. The page SHALL provide a live test: swiping the hardware slider opens a "Test – <use case>" popup driven by the highlighted gain on a 0..100 value that closes 1 s after release.

#### Scenario: Test area
- **WHEN** the Brightness slider is focused and the hardware slider is swiped
- **THEN** the "Test – Brightness" popup shows a value moving with the brightness gain

### Requirement: Unavailable entity
On press with a disabled (unavailable) target entity the UI SHALL send nothing and show the actionable notification "Touch slider is not available." / "%1 is not available. Please check your configuration." once; the warning SHALL not repeat until the entity became available again. Moves and release on an unavailable entity SHALL be ignored.

#### Scenario: Unavailable receiver
- **WHEN** the mapped receiver is unavailable and the slider is touched twice
- **THEN** the warning is shown once and no command is sent
