Stacked on `feat/button-simulator-repeat`. Phase 1 is the user-visible change and carries the
`CHANGELOG.md` entry. Phase 2 (docs) is file-disjoint from phase 1 and can run in parallel.
Phase 3 runs last. One commit per phase.

## 1. Escape as BACK in InputController (C++)

- [x] 1.1 In `InputController::eventFilter()` (`src/ui/inputController.cpp`), after the
      `blockInput` check: when `m_model` is `DEV` and the event is a key event for
      `Qt::Key_Escape`, accept a `ShortcutOverride` and return `true`; for a `KeyPress` or
      `KeyRelease`, send the same type with `Qt::Key_Exit` and the event's auto-repeat flag through
      `sendKeyEvent()` and return `true`. Other models and other keys take the existing code path.
- [x] 1.2 Extend `test/ui/test_input_controller.cpp`. Assert on the key events the source object
      itself receives (override `event()` of a test source object), because an event filter
      installed after the controller's runs before it and would also see the dropped Escape.
      Cases on `DEV`: an Escape press and release reach the source as a `Key_Exit` press and
      release, `keyPressed("BACK")` and `keyReleased("BACK")` are emitted, no Escape event reaches
      the source; a `ShortcutOverride` for Escape comes back accepted, one for another key (e.g.
      `Key_Down`) stays ignored; an auto-repeat Escape press becomes an auto-repeat `Key_Exit`
      press, and an auto-repeat Escape release becomes an auto-repeat `Key_Exit` release whose
      `keyReleased("BACK")` arrives deferred (not at once, then within the 150 ms deferral plus
      margin); with `blockInput(true)` nothing reaches the source. On a non-`DEV` model (`UCR3`):
      Escape reaches the source unchanged, no `BACK` signal, the `ShortcutOverride` stays ignored.
- [x] 1.3 `CHANGELOG.md`, "Unreleased", "Changed", below the button simulator entry: in the
      desktop simulator, the Escape key of the computer keyboard now acts as the BACK button.
- [x] 1.4 No registration needed: no new files; `testInputController` already compiles
      `inputController.cpp`.

## 2. Documentation

- [x] 2.1 `docs/key-navigation.md` §8: add that on `DEV` Escape acts as BACK on both paths
      (translated to `Key_Exit`, also for popups that would close on Escape), while Backspace and
      the keypad Enter stay unmapped.

## 3. Verification

- [x] 3.1 `make test` passes; run `testInputController` 5x to check it is not flaky.
- [x] 3.2 Format the changed lines (`git clang-format` on the changed files) and run `./cpplint.sh`.
- [x] 3.3 `make linux` and `make ucr2`; leave no translation churn (`git checkout
      resources/translations/`).
- [x] 3.4 Desktop walk with `UC_MODEL=DEV` against the Remote-Core Simulator, `uc.ui.input` at
      debug level, Escape and Backspace sent as real key events to the main window: Escape on a
      settings page leaves it (BACK pressed and released, no Escape in the log); held Escape repeats
      like a held BACK; Backspace on a settings page does nothing; the loading screen or the
      integration dropdown, if reachable, does not close on Escape but reacts as to BACK. The keys
      were sent through X11 (XTEST) to the activated main window, so Qt's shortcut handling ran.
      The loading screen only stays open for a moment in the Remote-Core Simulator, so a test
      helper opened it directly; on the base branch the same Escape closed it, with this change
      it stayed open and BACK was pressed and released.
- [x] 3.5 `openspec validate simulator-escape-as-back`; the `desktop-simulator` delta is synced
      into the living spec when the change is archived.
