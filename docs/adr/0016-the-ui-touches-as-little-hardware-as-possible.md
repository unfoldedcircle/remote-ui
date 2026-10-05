# ADR 0016 — The UI touches as little hardware as possible; the simulator is the device code with the hardware stubbed

|                |                                                                                                            |
| -------------- | ---------------------------------------------------------------------------------------------------------- |
| **Status**     | Accepted                                                                                                   |
| **Supersedes** | — (none)                                                                                                   |
| **Date**       | 2026-09-24                                                                                                 |
| **Deciders**   | Markus Zehnder                                                                                             |
| **Related**    | [0005](0005-core-api-over-websocket-ui-holds-no-business-logic.md), [0008](0008-remote-two-and-remote-3-with-feature-parity.md), `openspec/specs/hardware-platform`, `openspec/specs/desktop-simulator` |

## Context

The app grew out of the first YIO remote, which had no core process: the UI was the whole
software and talked to the hardware itself. On the Remote Two and Remote 3 the core owns the
device — power modes, battery, WiFi, system information, updates — and exposes it over the
Core-API (ADR 0005). What the UI drives itself is small: the haptic motor (a sysfs write, one
backend per model), the Remote 3 touch slider (an evdev device read by the UI), key and touch input
(evdev through Qt's platform plugin) and sound output (Qt Multimedia).

The desktop simulator is built from the same code base as the device binary, for other targets
(Linux, macOS and Windows on x64, macOS on Apple Silicon; the device binary is Linux aarch64). The
model, selected by `UC_MODEL`, only decides which hardware backends are created, and on a desktop
they are no-op stubs. That is deliberate: the simulator is only useful if what runs on the desk is
what runs on the remote.

## Decision

- **The UI controls as little hardware as possible; the hardware is the core's job.** The UI may
  drive directly, and only: the haptic feedback motor, input events (keys, touch, the touch
  slider) and sound output. Everything else — power, battery, WiFi, Bluetooth, display power,
  LEDs, system information, updates — is read and commanded through the Core-API, never through
  sysfs, D-Bus, sockets or processes of its own. A new hardware need is a Core-API feature request
  first.
- **One code base for the device and the simulator.** `UC_MODEL` selects the hardware backends and
  nothing else about behaviour; on a desktop the backends are stubs, and the simulator must stay
  as close to the device as the missing hardware allows. Behaviour that differs between the
  simulator and a device is a defect unless the `desktop-simulator` spec names it (the button
  simulator window, the hardware that is simply absent).
- **Model-specific code is confined** to the backend selection in `HardwareController` and to the
  few facts the models really differ in (screen geometry and rotation, the touch slider, the
  regulatory screen). It never branches the UI logic.
- **The code layout follows the boundary:** `src/hardware/` holds only the hardware backends
  (haptic, touch slider), the model and the `HardwareController` that selects them; the Core-API
  clients for WiFi, power, battery and system information live in `src/system/`.

## Consequences

- **Easier:** the UI needs no privileges, no hardware knowledge and no per-board drivers beyond
  three small backends; the core can change the hardware handling without a UI release; a
  simulator run exercises the real UI logic; a second device model is a backend and a few facts,
  not a fork.
- **Harder / accepted:** a hardware feature the UI wants waits for the Core-API to expose it; the
  simulator cannot exercise the direct backends, which need a device test.
