# Documentation

## Build Environment

- [Installation instructions](install.md): overview, common setup and links to the per-target guides for
  [Debian 13](install-debian-13.md) and [Ubuntu 22.04](install-ubuntu-22.04.md).
- [Compile a static desktop remote-ui app](static-compile.md): introduction and links to the per-target guides for
  [macOS](static-compile-macos.md) and [Debian 13](static-compile-debian-13.md).
- [Remote Two/3 device cross-compile & installation](cross-compile.md).
- Command line builds: `make` in the repository root lists the targets (`make linux`, `make linux-static`, `make ucr2`,
  `make test`, `make run-linux`, ...)
- Runtime environment files for starting the app are in [`scripts/env/`](../scripts/env/).

## Design

- [Onboarding](onboarding/README.md): Design documents for the localization steps of the first-run onboarding wizard.

## Implementation

- [Startup sequence](startup.md)
- [Key navigation](key-navigation.md): how d-pad / button presses reach the UI, input and focus ownership, the idioms a screen must follow, and the traps.

## Resources

- [Remote-Core Simulator](https://github.com/unfoldedcircle/core-simulator)
- [Unfolded Circle Core-APIs](https://github.com/unfoldedcircle/core-api)
