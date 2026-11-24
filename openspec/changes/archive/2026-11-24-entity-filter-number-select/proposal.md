## Why

The "Filters" sheet of the entity lists offers seven types: Button, Climate, Cover, Light, Media
player, Sensor and Switch. The number entity (#47) and the select entity are missing, so they can
only be listed unfiltered ([#65](https://github.com/unfoldedcircle/remote-ui/issues/65)).

## What Changes

- The "Filters" sheet gets a "Number" row with the number entity's default icon `uc:input-numeric`
  and a "Select" row with the select entity's default icon `uc:list-dropdown`, in alphabetical
  order between "Media player" and "Sensor". Ticking them filters the list with `entity_types`
  `number` and `select`.
- The type list of the sheet is sized to its rows, so the nine rows fit without the last one
  running into the bottom margin.
- The sheet is used by every entity list: "Add entities" of a page, creating and editing a group,
  and an integration's "Add entities" and "Manage entities".

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `entity-management`: "Search and type filter in entity lists" lists Number and Select among the
  types of the "Filters" sheet.

## Impact

- Hardware models: Remote Two and Remote 3 alike (ADR 0008).
- Core-API: none new. `entity_types` of `get_entities` and `get_available_entities` already accepts
  `number` and `select`; the UI already maps both types to these strings.
- Code: `src/qml/components/entities/EntityList.qml`, `resources/translations/en_US.ts`.
- Tests: none; the change is QML list data, the C++ type mapping is unchanged (see design.md).
- Docs: `CHANGELOG.md`.
- Third-party code and assets: none. Both icons are in the icon mapping and have a Free fallback.
