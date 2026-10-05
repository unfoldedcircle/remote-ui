## MODIFIED Requirements

### Requirement: Rendering and text setup
The UI SHALL render with OpenGL ES, use native text rendering with distance-field text disabled, load the icon font from the file named in `UC_ICON_FONT_PATH` when the firmware provides one and from the embedded resources otherwise, and use "Poppins" as the primary font and "Space Mono" as the secondary font. The corner radii used by the UI SHALL be 8 px (small) and 22 px (large). Text input SHALL use the Qt virtual keyboard with the embedded layouts and the "remotestyle" style.

#### Scenario: Startup
- **WHEN** the app starts
- **THEN** icons are loaded from the icon font — the firmware's file or the embedded one — and text is rendered natively
- **AND** every text field opens the embedded virtual keyboard
