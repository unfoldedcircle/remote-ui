# ADR 0011 — qmake builds the app and stays; CMake builds the tests

|                |                                                                                                            |
| -------------- | ---------------------------------------------------------------------------------------------------------- |
| **Status**     | Accepted                                                                                                   |
| **Supersedes** | — (none)                                                                                                   |
| **Date**       | 2026-09-24                                                                                                 |
| **Deciders**   | Markus Zehnder                                                                                             |
| **Related**    | [0002](0002-qt-5-15-lts-pinned.md), [0003](0003-static-aarch64-binary-from-the-public-toolchain.md), [0009](0009-unit-tests-for-new-logic-and-bug-fixes.md), [docs/cross-compile.md](../cross-compile.md) |

## Context

The app has been built with qmake since the first commit (2022-01): `remote-ui.pro` lists every
header, source, QML resource and translation explicitly, embeds the git version at qmake time and
runs `lupdate`/`lrelease` from the project file. The device toolchain image and the static
desktop images run `qmake CONFIG+=static CONFIG+=release && make` (ADR 0003), and the desktop
`Makefile` wraps the same call.

qmake is legacy: Qt 6 builds itself with CMake, recommends CMake for applications and only
maintains qmake. But the app is pinned to Qt 5.15 (ADR 0002), and Qt 5's CMake support is
incomplete — no `qt_add_qml_module`, no first-class handling of static plugins, resources and
translations comparable to Qt 6's — so a migration would re-create with custom CMake code what
`remote-ui.pro` already does, for no gain while Qt 5.15 lasts.

The unit tests, added in 2025-08, use CMake: `test/*/CMakeLists.txt`, one target per test area,
each recompiling the `src/*.cpp` files it needs and listing header-only files with
`Q_OBJECT`/`Q_GADGET` for AUTOMOC. CMake is the standard and is much simpler for tests. There is no
library the tests could link: qmake produces one executable, and CMake cannot consume its object
files.

## Decision

- **The app stays on qmake.** It is not migrated to CMake on Qt 5.15; the migration is not worth
  it. Nothing new is built with qmake beyond registering sources, resources and translations in
  `remote-ui.pro`.
- **Tests and tooling use CMake.** Every test target lists the sources it compiles; adding a
  dependency to code under test means editing that list. The recompiling is accepted: it costs
  build time, not correctness, and a library target would only come with a build system the app
  does not have.
- The `find_package(QT NAMES Qt5 Qt6 …)` lines in the test CMake files are not an invitation to
  use Qt 6; every target links `Qt5::`.
- A move off Qt 5.15 (superseding ADR 0002) is the moment to revisit this decision, not before.

## Consequences

- **Easier:** the device build recipe, the toolchain images and the version/translation steps are
  unchanged and proven; tests are plain CMake targets that CI and `make test` build the same way.
- **Harder / accepted:** two build systems have to be kept in sync — a new source file is
  registered in `remote-ui.pro` *and* in every test target that needs it, and forgetting either
  fails silently (not compiled) or late (link error). Test build times grow with every source a
  target recompiles. Running qmake rewrites the tracked `.ts` files, which every developer has to
  know about.
