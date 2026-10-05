# ADR Review Manifest

## ADR Review Completed

- Date: 2026-10-02
- Reviewer: Markus Zehnder
- Change: macos-desktop-builds

## In-Force ADR Context Reviewed

All ADRs under `docs/adr/` were read and the supersession graph built from their `Supersedes`
fields: nothing is superseded, so 0001–0016 are all in force. The ones that constrain this
change:

- `docs/adr/0002-qt-5-15-lts-pinned.md` — Qt 5.15 LTS only, the shipped patch level follows the
  device toolchain, the desktop development Qt moves first. The macOS build moves the macOS
  desktop to the same Qt 5.15.19 from source that Linux and the x64 toolchain images use, and
  the ADR is amended with that.
- `docs/adr/0003-static-aarch64-binary-from-the-public-toolchain.md` — one static binary with
  everything embedded, the desktop build a faithful simulator of the same code. The static macOS
  bundle applies the same practice to macOS, and the ADR is amended to list it.
- `docs/adr/0004-gpl-3-license-and-published-source.md` — no new third-party code: the build uses
  Qt under its existing licence with its own bundled libraries and a backport of a Qt commit, and
  the patch and configure scripts are published with the source.
- `docs/adr/0011-qmake-builds-the-app-cmake-builds-the-tests.md` — the macOS targets wrap qmake
  like the Linux ones; CMake still builds only the tests.
- `docs/adr/0016-the-ui-touches-as-little-hardware-as-possible.md` — the macOS simulator is the
  same code with the hardware stubbed; no hardware path changes.
- `docs/adr/0007` (one input idiom per screen) and `docs/adr/0008` (supported models) were
  checked and are untouched: no input path and no model changes.

## Repository-Level ADRs Created

**None.** No new durable architectural commitment was taken: building the desktop Qt from source
and shipping a static simulator were already decided. ADRs 0002 and 0003 have not been merged
yet — they arrive with the OpenSpec adoption in the same pull request as this change — so they
are amended with one dated sentence each rather than superseded, as the
`desktop-builds-and-qt-versions` change did. Neither the status, the numbering nor what was
decided changes, and `docs/adr/README.md` needs no edit:

- `docs/adr/0002-qt-5-15-lts-pinned.md` — amended 2026-10-02: the macOS desktop now also builds
  Qt 5.15.19 from source, static and shared, for Intel and Apple Silicon (commit `d2d6dae3`); CI
  and the device toolchain are unchanged.
- `docs/adr/0003-static-aarch64-binary-from-the-public-toolchain.md` — amended 2026-10-02: static
  desktop builds now exist for Linux x64, Windows x64 and macOS; the macOS one is a local build
  from a source-built Qt, not a public image and not a release artefact (commit `d2d6dae3`).

## Notes

Signing, notarization and a macOS CI job are not decided; should a macOS simulator become a
release artefact, that is a new change and possibly an amendment of ADR 0003's release practice.
