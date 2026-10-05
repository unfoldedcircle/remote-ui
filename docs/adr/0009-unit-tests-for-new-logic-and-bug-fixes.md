# ADR 0009 — Unit tests for new logic and regression tests for bug fixes

|                |                                                                        |
| -------------- | ---------------------------------------------------------------------- |
| **Status**     | Accepted                                                               |
| **Supersedes** | — (none)                                                               |
| **Date**       | 2026-09-17                                                             |
| **Deciders**   | Markus Zehnder                                                         |
| **Related**    | `test/` (QtTest, CMake), `openspec/specs/platform-constraints` |

## Context

The unit tests live under `test/` as QtTest targets built with CMake, one per area (for example
`testCore`, `testUiModels`, `testEntityController`, `testIconFont`); each target compiles the
sources it needs from `src/`. CI runs them on every pull request, but nothing states when a change must add tests, so
coverage depends on the author. Several regressions of the last releases were defects fixed
before and broken again.

## Decision

- **New logic is unit tested wherever a unit test is meaningful**: parsing, state handling,
  calculations, models and similar C++ logic. Code for which a unit test adds no value is not
  tested for its own sake.
- **A bug fix comes with a unit test that reproduces the defect** — it fails without the fix and
  passes with it — whenever the defect is in logic a unit test can exercise.
- A change's `tasks.md` lists these tests, including the edits to the test target's
  `CMakeLists.txt` source list.

## Consequences

- **Easier:** fixed defects stay fixed; reviewers can ask for the missing test by pointing here.
- **Harder / accepted:** logic that is buried in QML or tightly coupled to the UI controller may
  need to be moved into testable C++ first; each test target must list the sources and
  `Q_OBJECT` headers it compiles.
