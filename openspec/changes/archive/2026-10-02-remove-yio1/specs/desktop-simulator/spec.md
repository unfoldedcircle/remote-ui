## MODIFIED Requirements

### Requirement: Button simulator window
On the desktop model `DEV`, the only model that does not show regulatory information, the UI SHALL open a second window titled "Button simulator" with a black background, fixed at the main window's width and 0.95 times that width in height, showing a picture of the Remote Two keypad. The window SHALL be placed 60 px below the main window, left-aligned with it, or, when the main window's height plus the simulator's height exceed the available desktop height, directly to the right of the main window aligned with its top edge. Its content SHALL only be loaded while the window is visible. On Remote Two and Remote 3 the window SHALL stay hidden and empty.

#### Scenario: Tall desktop
- **WHEN** the simulator starts at scale 1 on a screen with at least 1306 px (850 + 456) of available height
- **THEN** a 480 x 456 "Button simulator" window appears 60 px below the main window

#### Scenario: Short desktop
- **WHEN** the combined window heights exceed the available desktop height
- **THEN** the button simulator window appears to the right of the main window, top edges aligned

#### Scenario: Device model
- **WHEN** `UC_MODEL` is `UCR2` or `UCR3`
- **THEN** only the main window is shown
