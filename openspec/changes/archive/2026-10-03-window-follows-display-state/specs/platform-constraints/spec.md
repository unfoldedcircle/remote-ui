## MODIFIED Requirements

### Requirement: Idle CPU usage

The UI MUST NOT assume that all CPU cores of the device are available to it: the firmware sets no
CPU limit today, but a firmware release can add one at any time. While the user does not interact
with the remote, it SHALL use as little CPU as possible, and while the display is off it SHALL stay
below 5 % of one core. Short bursts of high CPU usage, such as loading a page or processing a
burst of core events, are acceptable. The display is off in the Low_power mode; in the Idle mode
before it the display is only dimmed and the UI keeps rendering. In Low_power and Suspend the UI
hides its window, however the mode was reached, so Qt stops rendering (`power-and-battery`). While the display is off, animations SHALL stop and rendering SHALL stop
wherever Qt allows it without working against the framework. Processing of Core-API responses and
events SHALL continue, so the UI is up to date when the display turns on again.

#### Scenario: Display off

- **WHEN** the core reports the Low_power mode while the main page with animated content is shown
- **THEN** the window is hidden, no animation runs, the UI process stays below 5 % of one core
  apart from short bursts, and entity changes reported by the core are still applied

#### Scenario: Display dimmed

- **WHEN** the core reports the Idle mode
- **THEN** the UI keeps rendering the dimmed screen and runs no work beyond what the visible
  content needs

#### Scenario: Display on again

- **WHEN** the display turns on after an entity changed while it was off
- **THEN** the first rendered frame already shows the new state
