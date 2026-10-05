# ADR 0006 — `en_US.ts` is the only hand-edited translation; other languages come from the translation service

|                |                                                                       |
| -------------- | --------------------------------------------------------------------- |
| **Status**     | Accepted                                                              |
| **Supersedes** | — (none)                                                              |
| **Date**       | 2026-09-16                                                            |
| **Deciders**   | Markus Zehnder                                                        |
| **Related**    | `resources/translations/`, `openspec/specs/localization`, `AGENTS.md` |

## Context

The UI is translated into more than a dozen languages with Qt Linguist `.ts` catalogues, compiled
to `.qm` at qmake time and embedded into the binary. Translations are produced in a translation
service — currently SimpleLocalize, earlier Crowdin — not in this repository: the service is the master
of every non-English string, and the `l10n` branch receives its exports as pull requests
(today "Update translations from SimpleLocalize"). Running qmake shells out to `lupdate`/`lrelease`
and rewrites every `.ts` file in the tree, so any locally edited translation would be lost or
would conflict with the next export of the translation service.

## Decision

- `resources/translations/en_US.ts` is the **only** translation file ever edited or committed by a
  developer; it is generated from the source strings in the code.
- **Every user-visible string carries context for the translators** whenever its meaning is not
  obvious from the text alone — and on a remote with short labels it rarely is. Translators see
  each string on its own, without the screen: a missing context is the main cause of wrong
  translations ("Off" as a state or as an action, "Clear" as a verb or an adjective, what `%1`
  stands for). The context is written in the code, next to the string:
  - a translator comment `//:` on the line before `qsTr()` / `tr()`, saying where the text appears
    and what it does, and what every placeholder (`%1`, `%n`) is replaced with;
  - a disambiguation (the second argument of `qsTr()` / `tr()`) when the same English text needs
    different translations in different places.
  A reviewer asks for a missing comment like for a missing test.
- All other `*.ts` files are **owned by the translation service**, arrive through the `l10n` branch and are
  merged into `main` as pull requests; `.gitignore` excludes them and `.qm` files are never
  committed. A rename of a string is a delete plus a create in the translation service — avoid it, or
  rename there first.
- After a local build the `.ts` churn is reverted (restore `resources/translations/` from the
  index), never committed.
- A language is available on the device only when its `.qm` is listed in
  `resources/qrc/translations.qrc` (the app discovers languages by iterating `:/translations`).

## Consequences

- **Easier:** no merge conflicts between translators and developers; one place to write English;
  translators get the context with every string and translate it right the first time.
- **Harder / accepted:** an English string change is visible to non-English users only after the
  round trip through the translation service; writing a translator comment is part of adding a
  string; a language added to `TRANSLATIONS` in `remote-ui.pro` but not to the `.qrc` is silently
  unavailable — both lists must be edited together.
