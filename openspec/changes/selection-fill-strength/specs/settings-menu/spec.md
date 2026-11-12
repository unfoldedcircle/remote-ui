## ADDED Requirements

### Requirement: Selection background setting
"Display & Brightness" SHALL end with a "Selection background" slider of five levels, from "Off" (no fill) to the strongest fill of the design system, defaulting to the second highest level. Under the slider a preview row SHALL show "Selected entry" on the fill of the slider's level, following the slider while it moves. The UI SHALL store the level on the remote and apply it at once; it is not sent to the core.

#### Scenario: Slider moved to Off
- **WHEN** the user moves the "Selection background" slider to "Off"
- **THEN** the preview row shows no fill, and a selected row of the Settings menu is drawn without a fill

#### Scenario: Default level
- **WHEN** the setting has never been changed
- **THEN** the slider stands on the second highest level and the preview shows that level's fill

#### Scenario: Level kept after a restart
- **WHEN** the user picks a level and the UI restarts
- **THEN** the slider and every fill-style selection use the picked level

## MODIFIED Requirements

### Requirement: Immediate application of settings
Every switch and slider on the settings pages SHALL be sent to the core as soon as it is changed (sliders on release); there is no Save or confirmation. A rejected change SHALL show a warning notification with the core's message and keep the previous value. A setting the UI stores itself (see "Selection background setting") SHALL be applied as soon as it is changed and is not sent to the core.

#### Scenario: Value changed by the core
- **WHEN** the core reports a configuration change
- **THEN** the page shows the new value

### Requirement: Display & Brightness page
The page SHALL offer: "Auto brightness" switch ("Automatically adjust the display brightness based on ambient lighting conditions."); "Display brightness" slider 5–100; "Button backlight" switch ("When on, button backlight will automatically turn on in a dark room."); "Button backlight brightness" slider 0–100; "Selection background" slider (see "Selection background setting").

#### Scenario: Brightness slider moved
- **WHEN** the display brightness slider is moved
- **THEN** the brightness follows live and the final value is sent on release; on Remote Two the UI additionally dims itself according to the value
