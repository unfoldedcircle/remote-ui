# ADR 0004 — GPL-3.0-or-later, published source, user-installable custom builds

|                |                                                                                                            |
| -------------- | ---------------------------------------------------------------------------------------------------------- |
| **Status**     | Accepted                                                                                                   |
| **Supersedes** | — (none)                                                                                                   |
| **Date**       | 2026-09-16                                                                                                 |
| **Deciders**   | Markus Zehnder                                                                                             |
| **Related**    | [0003](0003-static-aarch64-binary-from-the-public-toolchain.md), [0010](0010-icon-font-free-embedded-pro-from-the-firmware.md), [CONTRIBUTING.md](../../CONTRIBUTING.md) |

## Context

The Remote-UI links Qt under the (L)GPL and is shipped inside a consumer device. Distributing a
statically linked Qt application means the whole program is distributed under a GPL-compatible
license and the users must be able to rebuild and replace it (the "installation information"
requirement of GPLv3). The source of every shipped release is therefore published in
`unfoldedcircle/remote-ui` on GitHub, and development is moving to that repository entirely.

## Decision

- The Remote-UI is licensed **GPL-3.0-or-later**. Every source file (C++ and QML) carries the
  copyright and `SPDX-License-Identifier: GPL-3.0-or-later` header; contributions are accepted
  under the same license.
- The source of every release (`vX.Y.Z` tag) is published. Nothing that may not be published —
  credentials, licensed assets, internal identifiers — ever enters the repository.
- The device offers an **API to install a custom UI build**, part of the
  [Core-API](https://github.com/unfoldedcircle/core-api); the toolchain is public (ADR 0003) and
  the build is documented (`docs/cross-compile.md`), so a user can build and install their own UI.
- New third-party code, libraries and assets must be compatible with Qt under GPL-3.0 and are
  added only after the lead developer has approved them; code copied from elsewhere keeps its
  license notice and is listed in the licenses shown in the About screen.
- Only redistributable assets are tracked and built into a binary. A licensed asset is never
  committed and never compiled in; the device firmware provides it as a file, as it does for the
  licensed Font Awesome icon font, while the repository and every binary hold Font Awesome Free
  (ADR 0010).

## Consequences

- **Easier:** a clear licensing story for a static Qt binary; contributors know the terms; the
  toolchain and app being public keeps the "custom firmware" community possible; every checkout
  builds and runs without any material that is not in the repository.
- **Harder / accepted:** a build without the licensed asset looks different (Solid instead of
  Light icons, placeholders for Pro-only icons, ADR 0010); secrets and licensed files have to be
  kept out of the repository entirely, because what is committed is published.
