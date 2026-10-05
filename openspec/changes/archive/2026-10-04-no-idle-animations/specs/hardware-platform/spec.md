## MODIFIED Requirements

### Requirement: Software dimming on Remote Two
On Remote Two the UI SHALL additionally dim its own output with a black overlay whose opacity is (100 - display brightness) / 100, covering the UI, the popups and the on-screen keyboard, animated over 300 ms. The animation SHALL finish also while the window is hidden with the display off, so that a brightness change with the display off leaves no animation running. The only such change is the configured brightness arriving when the UI starts with the display off: a brightness change through the Core-API, such as one in the web-configurator, makes the core turn the display on, on every model. Remote 3 and desktop SHALL not apply software dimming.

#### Scenario: Brightness 30 on Remote Two
- **WHEN** the display brightness is 30 on `UCR2`
- **THEN** a black layer with opacity 0.7 is drawn over the entire UI including the keyboard

#### Scenario: Remote 3
- **WHEN** the display brightness changes on `UCR3`
- **THEN** the rendered UI is unchanged; only the hardware backlight is affected

#### Scenario: Brightness change with the display off
- **WHEN** the UI starts on `UCR2` while the display is off and the configured brightness arrives
- **THEN** the overlay reaches its opacity while the window stays hidden, and no animation keeps the main thread busy
