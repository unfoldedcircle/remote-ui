## Context

The implementation is merged. Current State Analysis measured against the merge commit
`03598c57` (2026-09-18), with the state before it in brackets:

- **Engine lifetime.** `src/main.cpp:76` allocates the QML engine on the heap and
  `src/main.cpp:159` resets it after `app.exec()` returns (`src/main.cpp:151`), i.e. before the
  controllers declared above it go out of scope. _Before:_ the engine was a stack object declared
  next to the application, so it was destroyed *after* the controllers that own the objects behind
  the `ui`, `colors`, `fonts` and `resource` context properties and the QML singletons; every
  reference in the still existing scene turned null and hundreds of bindings re-evaluated, printing
  about 660 `TypeError: Cannot read property ... of null` lines per stop.
- **Window held weakly.** `src/ui/inputController.h:109` holds the filtered window in a `QPointer`.
  _Before:_ a raw pointer; with the engine destroyed first, the destructor called
  `removeEventFilter` on a deleted window and roughly one stop in three segfaulted (found with an
  AddressSanitizer build).
- **Sound effects.** `src/ui/soundEffects.cpp:27` returns with one info line when the directory is
  empty, `src/ui/soundEffects.cpp:32` warns and returns when it does not exist, the effect pointers
  are default-initialised to null (`src/ui/soundEffects.h:69-73`), `src/ui/soundEffects.h:59`
  exposes the availability and `src/ui/soundEffects.cpp:60` makes playing a no-op without effects.
  _Before:_ five `QSoundEffect` objects were created unconditionally and tried to decode
  `file:///click.wav` and friends at the root of the file system, logging five decoding errors on
  every desktop start.
- **Empty icon id.** `src/qml/components/Icon.qml:24` only calls the resource lookup for a
  non-empty identifier. _Before:_ every icon element without an icon produced an
  "Empty ID passed to getIcon()" line.
- **Translation.** `src/translation/translation.cpp:31-32` removes the translator without checking
  the result, and `src/translation/translation.cpp:40` spells "translation" correctly.
  _Before:_ a warning on every first language change, plus the typo "transaltion".

## Goals / Non-Goals

**Goals:** a log in which every remaining line means something, on the desktop and in the device
journal; a stop that is clean in the log and in the process exit.

**Non-Goals:** a logging policy or log-level review; the remaining desktop warnings that are
correct (the Remote-Core Simulator reporting the language `en_UK`, which has no translation, and
answering WiFi queries with 503).

## Decisions

- **Order the teardown instead of guarding the bindings.** The flood came from the destruction
  order, not from the QML. Making the engine a heap object that is reset before the controllers is
  one place to reason about; guarding hundreds of bindings against null would spread the fix over
  the whole UI and still leave the re-evaluation work at shutdown. _Alternative considered:_
  declare the engine before the controllers so the reverse declaration order destroys it first —
  rejected as too implicit; the explicit reset carries a comment saying why the order matters.
- **Do not create sound effects without a directory.** Refusing early is what makes the single log
  line possible and removes five decoder objects from the desktop run; playing then has to be a
  no-op, which is what the availability check provides.
- **No icon is a normal state, not an error.** The lookup is only asked when there is something to
  look up, which keeps the "unresolvable icon is logged" rule meaningful.

## Risks / Trade-offs

This change touches the static build's entry point and object lifetimes, a risky surface.

- [The engine is destroyed while a controller still calls into QML] → the reset happens after the
  event loop has returned, when nothing is dispatched any more.
- [Another raw pointer into the QML scene outlives the engine] → the one found (the filtered window)
  is a `QPointer`; the AddressSanitizer runs of the change found no other.
- [Sound effects silently stay off after a real configuration mistake] → a configured directory that
  does not exist is a warning naming the directory, not an info line.
- [Suppressing the translator removal result hides a real failure] → the failure that matters,
  loading the new translation, is still a warning.
- [Resource impact] → net positive and otherwise negligible: five `QSoundEffect` objects and their
  decoding attempts are gone on a desktop run, several hundred binding re-evaluations are gone from
  every shutdown; no timer, animation, cache or Qt module is added.

## Migration Plan

Merged in commit `03598c57`; verified by its test plan — 12 runs of `DEV` and `UCR2` stopped with
SIGTERM and SIGINT, idle and after d-pad navigation, with 0 type errors and exit code 0 every time
(about 660 lines per run before), five AddressSanitizer runs with no report, lint clean and all
test targets passing. **The device check is still to be done:** confirm that restarting the service
on a remote no longer writes the type error lines to the journal.

## Open Questions

None. No in-force ADR needs revisiting.
