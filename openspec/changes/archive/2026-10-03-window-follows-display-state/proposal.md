## Why

The UI hid its window only when the power mode changed from Idle to Low_power, the core's own
timeout path, and showed it only on Normal. A UI started while the display was off, or a Low_power
set through the Core-API while the display was on, left the window visible, so Qt kept rendering
with the display off until the next wake-up. A device measurement showed the render thread using
1.6 % of a core with the display off. Idle set through the Core-API from Low_power left the window
hidden while the display was dimmed.

## What Changes

- On a device the window is shown in Normal and Idle and hidden in Low_power and Suspend, decided
  by the current mode alone.
- On a desktop the window stays shown in every power mode.
- A unit test covers every way into and out of the display-off modes, a start while the display is
  off, and the desktop.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `power-and-battery`: "Window and touch handling per power mode" follows the current mode.
- `platform-constraints`: "Idle CPU usage" loses its known gap.
- `desktop-simulator`: "Hardware behaviour on desktop" keeps the window shown in every mode.

## Impact

- **Hardware models:** Remote Two and Remote 3; the desktop keeps its window.
- **remote-core dependency:** none.
- **Third-party code:** none added.
- **Code:** `src/system/power.{h,cpp}`, `src/hardware/hardwareController.cpp`, `src/qml/main.qml`,
  `test/hardware/test_power.cpp`, `test/hardware/CMakeLists.txt`, `CHANGELOG.md`.
- **Status:** merged on `main` as commit `b80b9a53`. This change carries the behaviour delta only
  and is archived on creation.
