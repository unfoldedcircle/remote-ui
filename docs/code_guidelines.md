# Code Guidelines

The Remote-UI is written in C++17 and QML on Qt 5.15 LTS. Qt 6 APIs, `QtQuick` 6 imports and
`qt_add_qml_module` are not available ([ADR 0002](adr/0002-qt-5-15-lts-pinned.md)).

Use the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) for C++ code,
with the following exceptions:

- indentation: 4 spaces (instead of 2)
- maximum line length: 120 (instead of 80)
- access modifiers (`public:`, `private:`) indented by one space
- consecutive declarations aligned
- `lowerCamelCase` method and function names (instead of `UpperCamelCase`)
- `lowerCamelCase` file names with `.cpp` and `.h` (instead of `.cc`); QML files `UpperCamelCase.qml`
- member variables prefixed `m_`, static members `s_` (instead of a trailing underscore)
- `#pragma once` instead of include guards
- namespaces `uc::`, `uc::ui::`, `uc::hw::`, `uc::core::`, `uc::integration::`, `uc::dock::`

The most important aspect is consistency, not following the guidelines to the letter.

- If in doubt, consult the Google style guide.
- Where the code base is not consistent, for example in naming constants, match the file you are
  working in.
- If existing code violates the guidelines, suggest a refactoring or a new exception to these
  guidelines in its own pull request; don't mix it into an unrelated change.

## Architecture Rules

The decisions behind these rules are recorded as [ADRs](adr/README.md).

- **The core owns the state.** The UI holds no business logic and keeps none of the core's data; it
  renders what the core reports and sends the user's intent
  ([ADR 0005](adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md)).
- **QML is presentation only.** Logic, state and models live in C++, where they can be unit tested
  ([ADR 0012](adr/0012-qml-is-presentation-only-logic-and-models-in-cpp.md)).
- **The latest request wins.** An answer that replaces state is checked against the latest request
  id or load generation before it is applied
  ([ADR 0017](adr/0017-the-latest-request-wins-late-answers-are-dropped.md)).
- **Unknown Core-API values are skipped.** A name from the core that is converted to an
  enumeration is checked and never stored as the unknown result
  ([ADR 0018](adr/0018-the-ui-tolerates-core-api-values-it-does-not-know.md)).
- **The UI touches as little hardware as possible.** Hardware backends exist per model in
  `src/hardware/`; WiFi, power, battery and system information come from the core through the
  Core-API clients in `src/system/`
  ([ADR 0016](adr/0016-the-ui-touches-as-little-hardware-as-possible.md)).
- **One input idiom per screen.** A screen is driven either by `ButtonNavigation` or by the QML
  focus chain, never both ([ADR 0007](adr/0007-one-input-idiom-per-screen.md),
  [key navigation](key-navigation.md)).

## Coding Conventions

- **Callbacks never outlive their objects.** A lambda connected to a signal, registered as a
  response handler or passed to a timer that uses `this` or another `QObject` gets that object as
  its context object, or checks a `QPointer` guard before it touches it. `QTimer::singleShot`
  always takes a context object, except on objects that live as long as the app.
- **Response handlers run once.** `onResult`, `onResponse` and `onResponseWithErrorResult` in
  `src/core/core.h` are one-shot: the first answer, error or request timeout settles the request
  and disconnects the handler. Both the success and the failure function end whatever the screen
  waits for, such as a spinner or a pending flag. A request that was never sent has the id -1 and
  registers no handler.
- **Worker threads never touch a `QObject`.** Work started on `QThreadPool` gets copies of its
  input and posts its result back to the GUI thread with a queued `QMetaObject::invokeMethod` on an
  object that outlives it; there, a `QPointer` decides whether the target still exists. The artwork
  decoding in `src/ui/entity/mediaPlayer.cpp` is the example.
- **List models keep the `QAbstractItemModel` contract.** Every change is wrapped in the matching
  `begin...` / `end...` calls or announced with `dataChanged` for exactly the rows that changed.

## Logging

- Log only through the categories declared in `src/logging.h` (`qCDebug(lcApp())`, ...), never
  through raw `qDebug()`; a new subsystem adds its category there. QML logs through `console.*`.
- Choose the level for the reader of the downloaded log: info for what explains the app's
  behaviour, warning for what went wrong, debug for detail that is only useful while diagnosing.
  The device stores info and above.
- **Never log secrets**: tokens, passwords, PINs, API keys, integration setup values and URLs that
  can carry credentials, in C++ and QML. Redact them as `<redacted>`.

See [ADR 0013](adr/0013-logging-through-qt-categories-to-journald.md) and the `platform-constraints`
spec ("No secrets in logs").

## QML

- QML has no linter or formatter configured: match the surrounding style.
- Entity detail screens are resolved through the registry in `src/ui/entity/entityScreens.cpp`
  (`EntityController.screenUrl(entity)`), never by building a path in QML.
- Use `fonts.iconFamily` for icons, never a font family name ([icon font](icon-font.md)).
- Extend a page's key handling with `buttonNavigation.extendDefaultConfig({...})`; assigning
  `defaultConfig` drops the `BACK` and `HOME` handlers ([key navigation](key-navigation.md)).

## Translations

A user-visible string, in C++ (`tr()`) and QML (`qsTr()`), gives the translators context whenever
its meaning is not obvious from the text alone, and on a remote with short labels it rarely is:
translators see each string on its own, without the screen
([ADR 0006](adr/0006-en-us-ts-is-the-only-edited-translation.md)). The context is written in the
code, next to the string:

- a translator comment `//:` on the line before the string, saying where the text appears, what it
  does and what every placeholder (`%1`, `%n`) is replaced with;
- a disambiguation, the second argument of `qsTr()` / `tr()`, when the same English text needs
  different translations in different places.

Only `resources/translations/en_US.ts` is ever edited or committed.

## Unit Tests

New logic and every bug fix get a unit test (QtTest), so a fixed bug cannot come back unnoticed
([ADR 0009](adr/0009-unit-tests-for-new-logic-and-bug-fixes.md)). Pure computations are extracted
into testable functions rather than tested through QML. A list model's test runs it under
`QAbstractItemModelTester` (`test/ui/test_dock_models.cpp`).

## 3rd Party Libraries

- Use what Qt 5.15 provides before adding a library.
- New third-party code, libraries and assets must be compatible with GPL-3.0-or-later and approved
  by the lead developer before they are added
  ([ADR 0004](adr/0004-gpl-3-license-and-published-source.md)).
- Copied code keeps its license notice and is listed in the licenses shown in the About screen.
- Licensed assets, such as the Font Awesome Pro font, are never committed or built in; the firmware
  provides them as files ([ADR 0010](adr/0010-icon-font-free-embedded-pro-from-the-firmware.md)).

## File Header

Every new C++ and QML file starts with a copyright notice and the SPDX license identifier. The
project is released under GPL-3.0-or-later.

```
// Copyright (c) {year} {person OR org} <{email}>
// SPDX-License-Identifier: GPL-3.0-or-later
```

Files written by the Unfolded Circle team use:

```
// Copyright (c) {year} Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later
```

## Source Code Formatter

A [ClangFormat](https://clang.llvm.org/docs/ClangFormat.html) configuration is included in the
project: [.clang-format](../.clang-format).

The code base is not uniformly formatted, so format only the lines you changed, never whole files.
`git clang-format` formats the staged lines in the working tree; stage the result again:

```shell
git add -u
git clang-format
git add -u
```

In Qt Creator the formatter can be set up under Preferences, Beautifier, Clang Format: use the
predefined style `File` and the fallback style `Google`.

The lints must pass, as they do in CI (`.github/workflows/code_guidelines.yml`):

```shell
./cpplint.sh
./design-check.sh
```

`design-check.sh` checks the QML sources against the rules of the [design system](design-system.md)
(section 8): no text below 22 px, only colour tokens, no `Qt.lighter` / `Qt.darker`. A file that
cannot follow a rule yet goes on the allow-list `tools/design-check-allow.txt` with its reason.
CI also fails when a source file name contains a space, and checks the embedded icon font and its
name mapping. Clang-Tidy is not run in CI; check new code with Clang-Tidy in Qt Creator (Debug
sidebar: Clang-Tidy).

## Commit Messages

Commits follow [Conventional Commits](https://www.conventionalcommits.org/):

```
<type>[optional scope]: <description>

<body: why the change was made>

<trailers>
```

- **Header:** the type (`feat`, `fix`, `docs`, `refactor`, `test`, `build`, `ci`, `chore`), an
  optional scope and a short description. Keep it at 50 characters; use up to 72 when 50 are
  simply not enough. A squash merge appends the pull request number.
- **Empty line** between the header and the body.
- **Body:** explain why the change was made: the problem, its effect for the user, and what the
  change achieves. Don't describe the technical implementation; the diff shows it. Wrap the body at
  72 characters.
- **AI attribution:** a commit that an agent or a large language model wrote or helped to write is
  attributed with a trailer naming the model, for example
  `Co-authored-by: Claude Opus 5.5 <noreply@anthropic.com>`.
- **No agent session information:** no session IDs, session links, prompts or transcripts in commit
  messages or pull request descriptions.
- **One logical change per commit.** A user-visible change carries its changelog entry in the same
  commit.

Example:

```
fix: keep the cover of the playing track

A slow artwork download could finish after the next track had started
and replace its cover with the previous one. The player now always
shows the cover of the track that is playing.

Co-authored-by: Claude Opus 5.5 <noreply@anthropic.com>
```

## Changelog

Every user-visible fix or feature gets an entry in `CHANGELOG.md`, in the same commit as the
change. The file follows [Keep a Changelog](https://keepachangelog.com/): entries go under
`## Unreleased` in `### Added`, `### Changed`, `### Removed` or `### Fixed`, written as
user-facing prose, not as commit messages.
