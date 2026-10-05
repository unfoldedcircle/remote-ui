## MODIFIED Requirements

### Requirement: Screen geometry
On a device the UI SHALL use the full geometry of the primary screen. On desktop (`DEV`) the UI SHALL use `UC_DISPLAY_WIDTH` x `UC_DISPLAY_HEIGHT` (default 480 x 850) logical pixels with `UC_DISPLAY_SCALE` as the global scale factor and high-DPI scaling enabled, the default scale being 1 on Linux and Windows and 0.5 on macOS; the window size and placement are specified in `desktop-simulator`. The desktop window title SHALL be "Remote Two simulator".

#### Scenario: Desktop defaults
- **WHEN** the app starts as `DEV` without display variables on Linux or Windows
- **THEN** the UI is laid out for 480 x 850 logical pixels and drawn at that size (see `desktop-simulator`)

#### Scenario: Desktop defaults on macOS
- **WHEN** the app starts as `DEV` without display variables on macOS
- **THEN** the UI is laid out for 480 x 850 logical pixels and drawn at half size, which is its intended physical size on a 2x Retina display

#### Scenario: Device
- **WHEN** the app starts on a device
- **THEN** the UI fills the screen at the panel's native resolution
