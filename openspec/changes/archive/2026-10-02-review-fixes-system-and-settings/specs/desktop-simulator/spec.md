## MODIFIED Requirements

### Requirement: Device models on a desktop
When `UC_MODEL` is `UCR2` or `UCR3` on a desktop the UI SHALL open only the main window, sized and fixed to the full geometry of the primary screen, with no desktop scale factor applied and `UC_DISPLAY_WIDTH`, `UC_DISPLAY_HEIGHT` and `UC_DISPLAY_SCALE` ignored. With `UCR2` the width and height SHALL be swapped and the UI rotated by -90 degrees as on the device. The device drivers SHALL be active: without a haptic device at `UC_HAPTIC_DEV_PATH` every haptic effect logs "Failed to write to haptic device"; with `UCR3` and no device at `UC_TOUCHSLIDER_DEV_PATH` the touch slider driver retries opening it every 1 s and logs one warning until the device appears, and again only after the device was found and has gone missing once more. Input then comes only from mouse, touch and the computer keyboard.

#### Scenario: Remote 3 on a desktop
- **WHEN** the app starts with `UC_MODEL=UCR3` on a 1920 x 1080 screen
- **THEN** one fixed 1920 x 1080 window opens with an unrotated UI and no button simulator

#### Scenario: Remote Two on a desktop
- **WHEN** the app starts with `UC_MODEL=UCR2` on a 1920 x 1080 screen
- **THEN** the UI is laid out for 1080 x 1920 and shown rotated by -90 degrees

#### Scenario: Missing touch slider device
- **WHEN** the app runs with `UC_MODEL=UCR3` and `UC_TOUCHSLIDER_DEV_PATH` does not exist
- **THEN** a "Touch slider device does not exist" warning is logged once, not once per second, while the driver keeps retrying every 1 s
