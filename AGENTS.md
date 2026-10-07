# Repository Guidelines for Agents

This file is for AI coding agents and human contributors working on the Remote-UI, the Qt 5.15 /
QML user interface of the Unfolded Circle Remote Two and Remote 3. Humans should also read
`README.md` and `CONTRIBUTING.md`. `CLAUDE.md` is a symbolic link to this file.

This file names the boundaries and the traps of the code base and links to the document that owns
each topic. When this file and a linked document disagree, the linked document wins and this file
is corrected.

## Critical Rules & Boundaries

### Always

- Keep changes small, focused, and traceable to a single feature, bug, or explicit request.
- Plan non-trivial work as an OpenSpec change before implementing it
  ([docs/workflow.md](docs/workflow.md)); a change that alters behaviour updates the living spec.
- Follow [docs/code_guidelines.md](docs/code_guidelines.md) and match the existing code style.
- Register every new file in the project and resource lists (see "Registering New Files").
- Build, run `./cpplint.sh`, `./design-check.sh` and the unit tests before opening a pull request.
- Add a `CHANGELOG.md` entry for every user-visible change, in the same commit.
- Write commit messages as described in the code guidelines, with the AI attribution trailer.

### Ask First

- Adding third-party code, libraries or assets
  ([ADR 0004](docs/adr/0004-gpl-3-license-and-published-source.md)).
- Changing what an accepted ADR decided; that needs a new ADR that supersedes it.
- Changing a budget or rule in the `platform-constraints` spec.
- Relying on a Core-API message, field or behaviour that the released core does not provide.
- Changing the hardware backends (`src/hardware/`), the build and release workflows
  (`.github/workflows/`, `Makefile`, `remote-ui.pro`) or the icon font pipeline (`tools/`).
- Large refactors, cross-cutting changes, or new dependencies of any kind.

### Never

- Use Qt 6 APIs, `QtQuick` 6 imports or `qt_add_qml_module`
  ([ADR 0002](docs/adr/0002-qt-5-15-lts-pinned.md)).
- Edit generated files by hand: the icon font `resources/icons/icon-font.ttf` and the name mapping
  `resources/icons/icon-mapping.json` ([docs/icon-font.md](docs/icon-font.md)).
- Commit licensed assets, such as the Font Awesome Pro font, or tokens, keys and other secrets.
  Never log secrets.
- Edit or commit any translation file other than `resources/translations/en_US.ts`, or commit the
  rewritten `.ts` files and generated `.qm` files a qmake run leaves behind.
- Modify third-party code in `3rd-party/` (the `QR-Code-generator` Git submodule).
- Edit a merged ADR; supersede it instead.
- Put agent session information, such as session IDs or session links, into commits or pull
  requests.

---

## Project Overview

The Remote-UI is written in C++17 and QML on Qt 5.15 LTS.

- On the remote it runs as one static aarch64 binary inside a systemd sandbox
  ([ADR 0003](docs/adr/0003-static-aarch64-binary-from-the-public-toolchain.md), `platform-constraints`
  "Runtime environment").
- It talks to remote-core over the Core-API WebSocket only. The core owns all state and business
  logic; the UI renders it and sends the user's intent
  ([ADR 0005](docs/adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md)).
- The Remote Two and the Remote 3 have feature parity
  ([ADR 0008](docs/adr/0008-remote-two-and-remote-3-with-feature-parity.md)).
- The desktop build is the simulator: the same code with the hardware backends stubbed, running
  against the [core-simulator](https://github.com/unfoldedcircle/core-simulator)
  ([ADR 0016](docs/adr/0016-the-ui-touches-as-little-hardware-as-possible.md), `desktop-simulator`
  spec).

What the app does is specified in the living specs under `openspec/specs/`, one capability per
folder; the non-functional rules (resource budgets, sandbox, logging, licensing, languages) are the
`platform-constraints` spec. Durable decisions are the ADRs in [docs/adr/](docs/adr/README.md).

---

## Project Structure & Module Organization

- `src/`
  - `core/`: the Core-API WebSocket client, its messages, enums and structs.
  - `ui/`: the UI controller, input controller, resources, notifications; entity classes and list
    models in `ui/entity/`, pages, groups and profiles in their own folders.
  - `qml/`: the screens and components; `qml/button-simulator/` is the desktop-only button window.
  - `hardware/`: the hardware backends per model (`ucr2/`, `ucr3/`): haptics and the touch slider.
  - `system/`: Core-API clients for WiFi, power, battery and system information.
  - `config/`, `dock/`, `integration/`, `softwareupdate/`, `translation/`, `voice.*`: the
    remaining features; `logging.h` declares the logging categories.
- `resources/`: the `.qrc` lists in `resources/qrc/`, icons and the generated icon font, images,
  and `translations/en_US.ts`.
- `test/`: unit tests, one CMake target per area in `test/<area>/CMakeLists.txt`.
- `openspec/`: living specs, changes (active and archived), the workflow schema and
  `config.yaml`, the project context injected into every artifact.
- `docs/`: guides and the ADRs.
- `tools/`: the icon font generator and the design system check (`design-check.py`, its allow-list). `scripts/`: environment scripts (`scripts/env/`) and the Qt
  build helpers (`scripts/qt/`).
- Repository root: `remote-ui.pro` (the app build), `Makefile` (entry point for all builds),
  `cpplint.sh`, `design-check.sh`, `.clang-format`, `CHANGELOG.md`.

### Registering New Files

- `remote-ui.pro` lists every header and source explicitly, without wildcards. A new
  `src/**/*.cpp` or `*.h` must be added to both `HEADERS` and `SOURCES`, or it silently isn't
  compiled.
- QML, images, keyboard layouts and translations are compiled in, and the app loads
  `qrc:/main.qml`. Every new `.qml` must be added to the matching file in `resources/qrc/`
  (`main.qrc` for app QML uses `alias=` with paths back to `../../src/qml/...`). A missing entry
  fails only at runtime with "file not found".
- A new entity detail screen is registered in `src/ui/entity/entityScreens.cpp` and in `main.qrc`;
  `testEntityScreens` checks both (`entity-management` spec).
- Unit tests link no library: each `test/*/CMakeLists.txt` compiles the `../../src/*.cpp` files it
  needs and lists the header-only files with `Q_OBJECT` / `Q_GADGET` for AUTOMOC. A new dependency
  of code under test means editing that list.
- A new language is listed both in `TRANSLATIONS` in `remote-ui.pro` and in
  `resources/qrc/translations.qrc`, or it is silently unavailable, and gets its keyboard layout in
  `resources/qrc/keyboard.qrc`.

---

## Commit & Pull Request Guidelines

- Commits: [code guidelines, "Commit Messages"](docs/code_guidelines.md#commit-messages) and
  ["Changelog"](docs/code_guidelines.md#changelog).
- Branches, pull requests and the disclosure of AI assistance: `CONTRIBUTING.md`, "Pull Request
  Best Practices" and "AI-Assisted Contributions".

---

## General Engineering Rules

Follow [docs/code_guidelines.md](docs/code_guidelines.md) for style, architecture rules and coding
conventions.

### Think Before Coding

**Do not assume. Do not hide confusion. Surface tradeoffs.**

Before implementing:

- State your assumptions explicitly. If uncertain, ask.
- If multiple interpretations exist, present them; do not pick silently.
- If a simpler approach exists, say so and propose it.
- If something is unclear, stop, name what is confusing, and ask.
- Check the living spec of the capability and the ADRs before changing behaviour.

### Simplicity First

**Minimum code that solves the problem. Nothing speculative.**

- No features beyond what was requested.
- No abstractions for single-use code.
- No "flexibility" or "configurability" that was not requested.
- If you wrote 200 lines and it could be 50, rewrite it.

### Surgical Changes

**Touch only what you must. Clean up only your own mess.**

- Do not "improve" adjacent code, comments, or formatting.
- Do not refactor things that are not broken.
- Match the existing style, even if you would do it differently.
- If you notice unrelated dead code, mention it in the pull request; do not delete it unless asked.
- Remove imports, variables, or functions that **your** changes made unused.

The test: every changed line should trace directly to the request.

---

## Build Environment

After cloning, update the Git submodule:

```shell
git submodule update --init --recursive
```

`make` in the repository root lists every target (`make linux`, `make macos`, `make test`,
`make ucr2`, ...); the install and build guides are indexed in [docs/README.md](docs/README.md).
Device builds are static aarch64 builds in the toolchain container only
([docs/cross-compile.md](docs/cross-compile.md)).

- **qmake rewrites the working tree.** It runs `lupdate` and `lrelease`, which rewrite every
  tracked `resources/translations/*.ts` and create untracked `.qm` files. Never commit that churn:
  `git checkout resources/translations/` after a build. Without `lupdate` / `lrelease` on `PATH`
  qmake only warns, and the build then fails on the missing `.qm` files.
- **Qt version:** `. scripts/env/qt-version.sh [version]` selects one of several installed Qt 5.15
  versions for the shell (`desktop-simulator` spec, "Qt version selection for a desktop build").
- **Version:** it comes from `git describe --match "v[0-9]*" --tags` at qmake time. A `vX.Y.Z` tag
  triggers a release; a push to `main` produces a `latest` prerelease.

### Desktop Simulator

The simulator needs the [core-simulator](https://github.com/unfoldedcircle/core-simulator) and
`UC_TOKEN_PATH`; `README.md` lists the environment variables. Only `UC_MODEL=DEV` is supported on a
desktop.

Read [docs/key-navigation.md](docs/key-navigation.md) before changing or testing key handling. Two
traps catch every newcomer:

- In `DEV` the button simulator window takes the window focus, so a page that navigates by QML
  focus looks dead on the desktop while it works on the device. Don't conclude such a page is
  broken from a `DEV` run.
- Every key press travels two paths, `ButtonNavigation` and the QML focus chain, and a screen uses
  only one idiom ([ADR 0007](docs/adr/0007-one-input-idiom-per-screen.md)).

---

## Source Code Format

Format only the lines you changed, then run the lint as CI does
([code guidelines, "Source Code Formatter"](docs/code_guidelines.md#source-code-formatter)):

```shell
git add -u && git clang-format && git add -u
./cpplint.sh
./design-check.sh
```

---

## Testing

Unit tests use QtTest and are built with CMake in `test/`, independently of the qmake app build
([ADR 0011](docs/adr/0011-qmake-builds-the-app-cmake-builds-the-tests.md)). What needs a test:
[code guidelines, "Unit Tests"](docs/code_guidelines.md#unit-tests).

```shell
make test                     # build and run all unit tests against the selected shared Qt
cd test/build && ctest -R testCore    # run one target again
```

The tests build for the host architecture. Never hard code `CMAKE_OSX_ARCHITECTURES`; an x86_64-only
Qt on Apple Silicon takes `-D CMAKE_OSX_ARCHITECTURES=x86_64` on the command line. The
`find_package(QT NAMES Qt6 Qt5 ...)` in the test CMake files is misleading: every target links
`Qt5::`.

---

## Other Documentation

- Project README: `README.md`
- Contributor guidelines: `CONTRIBUTING.md`
- Security policy and vulnerability reporting: `SECURITY.md`
- Code guidelines: [docs/code_guidelines.md](docs/code_guidelines.md)
- Development workflow (OpenSpec and ADRs): [docs/workflow.md](docs/workflow.md),
  [openspec/README.md](openspec/README.md); the OpenSpec commands are generated per tool and not
  tracked: `npx @fission-ai/openspec@1.14.1 init --tools <tool>`
- Architecture Decision Records: [docs/adr/](docs/adr/README.md)
- All other guides (build, install, key navigation, icon font, start-up): [docs/README.md](docs/README.md)
