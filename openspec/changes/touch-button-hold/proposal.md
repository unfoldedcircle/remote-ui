## Why

A page button on the touch screen only sends a single command, at the release, however long the
finger stays down. Users can hold a physical button to keep the volume going or to search, but
not a page button, and they have asked for it.

This change makes a long touch on a page button of a remote behave like a long press of a physical
button. It is stacked on `ir-button-tap-and-hold` (#93) and reuses the remote entity's
mapped-button handling from it.

## What Changes

- A page button whose command goes to a **remote entity**, on the remote's screen and in an
  activity:
  - **a tap** sends exactly what it sends today, at the release;
  - **a hold** starts repeating 600 ms after the touch-down and repeats every 150 ms, with the same
    messages a held physical button sends. For a simple command that is
    `remote.send {command, repeat: 4}` (with `press: true` on the remote's screen), then
    `remote.stop_send` when the finger is lifted;
  - a second haptic click marks the start of the hold.
- **A touch never sends anything at touch-down**, because it can still turn into a page swipe.
  - Before its hold starts, a touch that turns into a swipe is cancelled and sends nothing.
  - Once the hold has started, the button keeps the finger: a sideways drift no longer swipes the
    page.
  - A touch whose button disappears (the screen closes, the pages reload), or during which the
    remote stops taking touch input (low power, a blocking wait), is cancelled: `remote.stop_send`
    if the hold had started, and no tap afterwards.
- **A hold always ends,** at the latest after 30 s, even if its release is lost.
- **Which commands repeat:** a simple command, or a `remote.send` / `remote.send_cmd` whose only
  param is `command`, which is then held like that simple command. Every other command (sequences,
  `remote.on` / `off` / `toggle`, commands with their own params) and every power command (named
  `on`, `off`, `toggle`, or with "power" in its name) keeps the single command at the release.
- **One constant for the timing:** the 600 ms delay and the 150 ms period are defined once, so they
  can be adjusted in one place or become a setting later.
- **Unchanged:**
  - page buttons of other entity types in an activity (media players, lights, macros, activities):
    a single command at the release;
  - every tap message;
  - the physical buttons.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `remote-entity`:
  - "Tapping a page button" no longer says that holding doesn't repeat;
  - new requirements "Holding a page button", "Cancelled page button touch" and "Page button
    commands that don't repeat".
- `activities`: new requirement "Holding a page button of a remote", which refers to
  `remote-entity`.

## Impact

- **Hardware models:** Remote Two and Remote 3 alike. The haptic click goes through the existing
  `Haptic` backend.
- **Core-API:** the messages a held physical button already sends (`remote.send` with `repeat`,
  `remote.stop_send`). No new message or field. The page item has no per-button repeat setting.
- **Core dependency:** the remote-core IR toggle-bit fix, which ships with #93 in the same
  firmware release.
- **Code:**
  - `src/ui/entity/remote.h` / `.cpp`: touch entry points with the hold timer, the timing
    constants, the hold limit, and the rule which commands repeat;
  - `src/ui/inputController.h` / `.cpp`, `src/ui/entity/entityController.h` / `.cpp`,
    `src/ui/uiController.cpp`: end every touch hold when touch input gets blocked;
  - `src/qml/components/ButtonIcon.qml`: forwards press, release, cancel and destruction;
  - `Remote.qml` and `Activity.qml`: pass the remote entity to their page buttons.
- **Tests:** `test/ui/test_remote_entity.cpp`, `test_entity_controller.cpp`,
  `test_input_controller.cpp` (existing targets).
- **Third-party code and assets:** none.
- **Docs:** `CHANGELOG.md`.
