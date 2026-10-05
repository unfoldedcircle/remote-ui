## MODIFIED Requirements

### Requirement: Button-navigation handler dispatch
A button-navigation instance SHALL handle a key only when the announced owner is its scope, or when `overrideActive` is true, in which case its handlers fire for any owner. `overrideConfig` handlers SHALL take precedence over `defaultConfig` handlers for the same key and type. There SHALL be no switch that silences all handlers of an instance at once: a screen that has to stop reacting to some keys SHALL drop the configuration that declares them, so that the handlers it did not drop — BACK and HOME above all — keep working and the screen can always be left.

#### Scenario: Owner mismatch
- **WHEN** a key is announced for owner A and a button navigation whose scope is B has a handler for it
- **THEN** the handler of B does not run

#### Scenario: Override active
- **WHEN** a button navigation has `overrideActive: true` (e.g. the global VOICE handler in the main window)
- **THEN** its handlers run regardless of which layer owns the input

#### Scenario: Screen blocks its command keys
- **WHEN** a screen drops the override configuration that carries its command keys, for example while its entity is unavailable
- **THEN** those keys do nothing while the base configuration's BACK and HOME still close the screen
