## MODIFIED Requirements

### Requirement: Window and touch handling per power mode
On a device the UI window SHALL be shown exactly while the display is on: shown in Normal and Idle, hidden in Low_power and Suspend. Whether it is shown SHALL depend on the current mode only, not on the mode before it, so that every way into a display-off mode hides it: the core's timeout from Idle, a mode set through the Core-API (the core can go from Normal to Low_power directly), and a UI start while the display is off, which the UI sees as a change from its start mode Normal. Nothing SHALL be rendered while the window is hidden. On a desktop, where the app does not own the display, the window SHALL stay shown in every power mode. On a device (not on desktop) the UI SHALL drop all touch and mouse input while the mode is Low_power and accept it again when the mode is Normal.

#### Scenario: Entering low power
- **WHEN** the mode changes from Idle to Low_power on a device
- **THEN** the window is hidden and touches are ignored

#### Scenario: Waking up
- **WHEN** the mode becomes Normal
- **THEN** the window is shown and touch input works again

#### Scenario: Low power set through the API
- **WHEN** an API client sets Low_power on a device while the mode is Normal
- **THEN** the window is hidden and nothing is rendered until the display is on again

#### Scenario: Start while the display is off
- **WHEN** the UI starts on a device while the remote is in Low_power
- **THEN** the window is hidden as soon as the core reports the mode

#### Scenario: Dimmed through the API
- **WHEN** an API client sets Idle on a device while the mode is Low_power
- **THEN** the window is shown with the dimmed display

#### Scenario: Desktop
- **WHEN** the core simulator reports Low_power on a desktop
- **THEN** the window stays shown
