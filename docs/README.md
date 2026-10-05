# Documentation

The index of project documentation. **New here? Start with the
[development workflow](workflow.md).**

## Where things live

- **Behaviour → [`openspec/`](../openspec/README.md).** What the app _does_, as requirements +
  WHEN/THEN scenarios, one capability per folder under `openspec/specs/`. It changes through an
  OpenSpec _change_, not through a doc edit.
- **Architecture & reference → this folder (`docs/`).** How it is built and how to work in it:
  build environment, startup, key navigation, design notes. Prose, not WHEN/THEN.
- **Decisions → [`docs/adr/`](adr/README.md).** One durable decision per file, immutable.

## Process — how we work

- [Development workflow](workflow.md): plan-first / spec-driven flow. The entry point.
- [OpenSpec](../openspec/README.md): changes and living specs — the plans agents implement against.
- [Architecture Decision Records](adr/README.md): one decision + rationale per file.
- [Code guidelines](code_guidelines.md): code style, architecture rules, coding conventions, commit messages and
  the changelog.

## Build Environment

- [Installation instructions](install.md): overview, common setup and links to the per-target guides for
  [Debian 13](install-debian-13.md) and [Ubuntu 22.04](install-ubuntu-22.04.md).
- [Compile a static desktop remote-ui app](static-compile.md): introduction and links to the per-target guides for
  [macOS](static-compile-macos.md), [Debian 13](static-compile-debian-13.md) and the experimental
  [Windows x64 cross-build](static-compile-windows.md).
- [Remote Two/3 device cross-compile & installation](cross-compile.md).
- Command line builds: `make` in the repository root lists the targets (`make linux`, `make linux-static`,
  `make linux-x64`, `make ucr2`, `make windows-x64`, `make test`, `make run-linux`, ...)
- Runtime environment files for starting the app are in [`scripts/env/`](../scripts/env/).

## Design

- [Onboarding](onboarding/README.md): Design documents for the localization steps of the first-run onboarding wizard.

## Implementation

- [Icon font](icon-font.md): the Free edition built into every binary, the licensed edition the firmware installs and
  names in `UC_ICON_FONT_PATH`, how missing icons fall back, and how the font is rebuilt.
- [Startup sequence](startup.md)
- [Measuring the resource budgets](measuring-resource-usage.md): for the maintainers, how to take idle CPU,
  memory, start-up, frame rate, latency and binary size numbers on a device, matching the `platform-constraints`
  budgets; the [results](measurement-results.md) so far.
- [Key navigation](key-navigation.md): how d-pad / button presses reach the UI, input and focus ownership, the idioms a screen must follow, and the traps.

## Resources

- [Remote-Core Simulator](https://github.com/unfoldedcircle/core-simulator)
- [Unfolded Circle Core-APIs](https://github.com/unfoldedcircle/core-api)
