## Why

Device measurements showed two animations that kept running where nothing was visible. The animation
of the status bar's connecting indicator ran all the time, also while the indicator was hidden,
because its `running` binding resolved to the window's `show()` function: it kept the render loop
busy with the display on and woke the main thread with the display off. On a Remote Two, the
configured brightness arriving at a start with the display off started the software dimming's fade
as a render-thread animator, which cannot finish while the window is hidden, so Qt drove its timer
on the main thread at about 64 wakeups per second until the display turned on.

## What Changes

- The connecting indicator's animation runs only while the indicator is shown.
- The Remote Two's dimming fade runs on the main thread and finishes also while the window is
  hidden.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `integrations`: "Integration and driver state follow core events", scenario "A driver is
  connecting", says when the indicator animates.
- `hardware-platform`: "Software dimming on Remote Two" finishes its fade with the display off.

## Impact

- **Hardware models:** both; the dimming fix affects the Remote Two only.
- **remote-core dependency:** none.
- **Third-party code:** none added.
- **Code:** `src/qml/components/StatusBar.qml`, `src/qml/main.qml`, `CHANGELOG.md`.
- **Status:** merged on `main` as commits `30309ded` (indicator) and `a9c281c4` (dimming). This change
  carries the behaviour delta only and is archived on creation. The measurements are in
  `docs/measurement-results.md`.
