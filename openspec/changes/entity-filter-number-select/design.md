## Context

See proposal.md - Why. Current state at `b0020e9`:

- The types of the "Filters" sheet are a static `ListModel` in
  `src/qml/components/entities/EntityList.qml`. Each row carries a name, an icon and an
  `EntityTypes` value.
- Toggling a row calls `setEntityType` / `removeEntityType` of the list model
  (`ConfiguredEntities`, `AvailableEntities`), which turn the enum into its lower-case name for
  `entity_types`. `EntityTypes.Number` and `EntityTypes.Select` already become `number` and
  `select`, so the C++ side needs no change.
- The type list is sized `(count + 1) * 60`. With seven rows of 60 px and a spacing of 10 that is
  exactly the 480 px the rows need; with nine rows it would be 600 px for 620 px of rows.

## Goals / Non-Goals

**Goals:** Number and Select can be picked in the sheet of every entity list.

**Non-Goals:** No other missing type is added to the sheet (activity, macro, remote and voice
assistant are not entity-list types today). No change to the order of the existing types.

## Decisions

- **Rows in alphabetical order**, between "Media player" and "Sensor", as the existing rows are
  ordered. Alternative: append at the end; rejected, the list reads alphabetically today.
- **Icons are the entity types' default icons**, `uc:input-numeric` and `uc:list-dropdown`, as the
  issue asks for the number entity. Both are in the icon mapping with a Free fallback (`hashtag`,
  `square-caret-down`).
- **The list height is the sum of its rows and spacings**, `count * (60 + spacing) - spacing`. It
  is the same 480 px for seven rows, so nothing else moves, and grows with the model.
- **No unit test** (ADR 0009): the change is QML list data. The type-to-string mapping it relies on
  is existing C++ that is not changed.
- **Translator comments** on both names: "Number" is not a phone number, and "Select" is a noun
  here, not the verb of "Select all" in the same file.

## Risks / Trade-offs

- [Taller sheet] With nine rows the sheet is about 140 px taller and still fits the 850 px high
  screen of both models. → Checked in the desktop simulator.

## Migration Plan

None. The UI keeps no state of the filter; a list opens with no type ticked.
