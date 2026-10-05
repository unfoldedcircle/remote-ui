## Why

The searchable entity list — "Add entities" on a page, the entity manager of an integration, and the
entity picker of a group — could show rows that do not match what is in the search field, came back
with a stale search term and stale filter check marks when it was reopened, and made the integration
fetch its complete entity list again for every typed character.

## What Changes

- Answers to a search that has been superseded are discarded: the list only accepts the answer to
  the request it is currently waiting for, so it always ends up matching the text in the field. This
  is the same one-request-at-a-time guard the media browser search already uses, and it now lives in
  the shared model of both entity lists, the available and the configured one.
- Reopening the list resets the search field and the type filters, so the header no longer claims a
  search and a filter that are not applied. Every entry point goes through the list's own open
  routine; the search field also restores its placeholder and its magnifier icon when it is cleared
  from the outside.
- The forced reload of an integration's entities happens only when the list is opened. Searching,
  filtering and loading a further page use the list the core already has.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `entity-management`: how the available and configured entity lists load, and what a search, a
  filter change and reopening the list do.

## Impact

- **Hardware models:** both, Remote Two and Remote 3; the lists are identical on both.
- **remote-core dependency:** none. The same Core-API requests are used, `get_entities` and
  `get_available_entities`; only the `force_reload` flag of the latter is now sent as true just for
  the first load of a list instead of for every request. No remote-core version dependency is
  introduced.
- **Third-party code:** none added.
- **Code:** `src/ui/entity/entities.{h,cpp}`, `src/ui/entity/availableEntities.{h,cpp}`,
  `src/ui/entity/configuredEntities.cpp`, `src/qml/components/entities/EntityList.qml`,
  `src/qml/components/SearchField.qml`, `src/qml/components/group/GroupAdd.qml`,
  `test/ui/CMakeLists.txt`, `test/ui/test_entities_stale_response.cpp` (new), `CHANGELOG.md`.
- **Keypad navigation is untouched**: no button-navigation configuration and no focus chain changed.
- **Status:** the implementation is **merged** on `main` as commit `3778fc27`.
  This change carries the behaviour delta only and is archived on creation.
