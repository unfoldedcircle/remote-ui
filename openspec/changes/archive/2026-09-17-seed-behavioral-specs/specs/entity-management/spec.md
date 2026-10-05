## ADDED Requirements

### Requirement: Entity identity and shared entity object
Every configured entity SHALL be identified by its `entity_id` and held exactly once by the UI; every tile, group row, widget and control screen that shows the same id SHALL show the same live entity, so a change is visible everywhere at once. An entity carries its multilingual name, icon, area, type, device class, features, state and type-specific attributes, options and integration id. A screen that asks for an entity which is not loaded yet SHALL show a placeholder (no name, greyed out) and fill in as soon as the entity is loaded.

#### Scenario: Same entity on two pages
- **WHEN** a light is placed on two pages and the core reports it as `ON`
- **THEN** both tiles show the light as On

#### Scenario: Tile before its entity is loaded
- **WHEN** a page is shown before the entity of one of its tiles has been loaded
- **THEN** the tile is empty and greyed out, and it fills in when the entity arrives

### Requirement: Supported entity types and default icons
The UI SHALL support the entity types `activity`, `button`, `climate`, `cover`, `light`, `macro`, `media_player`, `remote`, `select`, `sensor`, `switch` and `voice_assistant`. An entity without an icon SHALL get a default icon per type: button and switch `uc:power-on`, climate `uc:climate`, cover `uc:blind`, light `uc:light`, media player `uc:music`, sensor `uc:sensor`, remote `uc:remote`, activity and macro `uc:activity`, voice assistant `uc:microphone`, select `uc:list-dropdown`. An entity of any other type SHALL still be kept as an unsupported entity: it shows its name and its icon (default `uc:warning`), has no state, no features and no attributes, and offers no quick action.

#### Scenario: Light without icon
- **WHEN** the core delivers a light without an `icon`
- **THEN** the light is shown with the `uc:light` icon

#### Scenario: Unknown entity type
- **WHEN** the core delivers an entity of type `water_heater`
- **THEN** the entity is kept and shown with its name and the `uc:warning` icon, without a state and without a quick action

### Requirement: Device class fallback
An entity's device class SHALL select its control screen. A missing or unknown device class SHALL fall back to the type's default: button `button`, switch `switch`, climate `climate`, cover `blind`, light `light`, media player `speaker`, remote `remote`, sensor `custom`, activity `activity`, macro `macro`, select `select`. Covers with device class `shade` SHALL be shown as `blind`, and `door` and `gate` as `window`.

#### Scenario: Unknown media player class
- **WHEN** a media player reports device class `projector`
- **THEN** it is handled as a `speaker`

#### Scenario: Gate cover
- **WHEN** a cover reports device class `gate`
- **THEN** its control screen is the window cover screen

### Requirement: Bulk load of configured entities
On every successful authentication, and whenever the core signals that the entities have to be reloaded, the UI SHALL load all configured entities with `get_entities` pages of 100, starting at page 1 and requesting the next page only after the previous one arrived, until page ceil(count / limit) is reached. An entity already known SHALL be updated in place (as for an entity change) instead of being recreated. After the last page every locally known entity that was not in any page SHALL be removed as if the core had deleted it, and the "all entities loaded" signal SHALL start loading the profile and its pages. Starting a new load or losing the connection SHALL invalidate a running load: pages and errors arriving for it are ignored and it removes nothing. A failed page SHALL end the load without removing any entity, and the profile SHALL still be loaded.

#### Scenario: 250 configured entities
- **WHEN** the core reports a count of 250 with limit 100
- **THEN** pages 1, 2 and 3 are requested one after another and the profile is loaded after page 3

#### Scenario: Entity deleted while disconnected
- **WHEN** an entity was deleted in the Web Configurator while the remote was offline and the remote reconnects
- **THEN** after the last page the entity disappears from the UI

#### Scenario: Connection lost during the load
- **WHEN** the connection drops after page 1 and the pages of the old load still arrive
- **THEN** they are ignored and a new load starts from page 1 after the next authentication

#### Scenario: Page request fails
- **WHEN** page 2 is answered with an error
- **THEN** no further page is requested, no entity is removed and the profile is loaded anyway

### Requirement: Entity change events
The UI SHALL apply `entity_change` events by `event_type`: `NEW` adds the entity (or updates it in place when the id is already known), `CHANGE` updates a known entity in place and is ignored for an unknown id, `DELETE` removes the entity. A `NEW` or `CHANGE` event without `new_state`, or a `DELETE` event without `entity_id`, SHALL trigger a full reload of all entities. An in-place update SHALL apply only what the event carries: a non-empty name replaces the whole name map, a non-empty icon replaces the icon, each attribute is applied with `state` applied last, a present `features` list replaces the feature list (not for sensors and selects), and non-empty options are applied. Area, device class, type, integration and the `enabled` flag of an existing entity SHALL NOT be changed by an update. An attribute value the entity type does not know, including an unknown state value, SHALL be ignored.

#### Scenario: State change
- **WHEN** a `CHANGE` event carries only `attributes.state` = `OFF` for a switch
- **THEN** the switch shows Off and keeps its name, icon and features

#### Scenario: State applied after the other attributes
- **WHEN** one event carries `step` and `state` for an activity
- **THEN** whatever reacts to the new state already sees the new step

#### Scenario: Event for an entity that is not loaded
- **WHEN** a `CHANGE` event arrives for an unknown entity id
- **THEN** nothing happens

#### Scenario: Event without entity data
- **WHEN** a `CHANGE` event without `new_state` arrives
- **THEN** all entities are reloaded as on a new connection

### Requirement: Entity features
Feature names from the core SHALL be matched case-insensitively against the feature set of the entity type. Unknown feature names SHALL be dropped with a log entry while the known ones are kept. A feature list that is delivered SHALL replace the previous list completely. Asking whether an entity has all or any of an empty list of features SHALL answer no. An entity whose core data carried no features has an empty feature list until a later load or event delivers them.

#### Scenario: Partly unknown features
- **WHEN** a remote reports features `send_cmd`, `on_off` and `foo`
- **THEN** the remote has `send_cmd` and `on_off`, and `foo` is ignored

#### Scenario: Feature removed by the integration
- **WHEN** a light that had `dim` and `color` receives an update with features `on_off` only
- **THEN** the light no longer has `dim` or `color`

### Requirement: Refresh when a control screen opens
Opening the control screen of any entity SHALL request that entity with `get_entity` and apply the answer in place, including its features and options. A failed refresh SHALL only be logged; the screen keeps the data it has.

#### Scenario: Open a light
- **WHEN** the user opens a light's control screen
- **THEN** `get_entity` is sent for it and its current state and features are applied when the answer arrives

### Requirement: Entity removal
When an entity is removed (core `DELETE` event, missing after a bulk load, or its integration deleted) the UI SHALL immediately stop resolving its id, drop all of its pending commands and clear its loading indicator, remove it from the running activities, and dispose of its object 100 ms later. An entity added again with the same id SHALL get a new object right away. Removing tiles from pages and groups is described in the `pages` and `groups` capabilities.

#### Scenario: Deleted while a command is pending
- **WHEN** an entity with a spinning loading indicator is deleted
- **THEN** its pending command is dropped and no loading indicator keeps running

#### Scenario: Re-added with the same id
- **WHEN** an entity is deleted and a `NEW` event with the same id follows within 100 ms
- **THEN** the new entity is shown and is not affected by the disposal of the old one

### Requirement: Entities of a deleted integration
When the core reports an integration as deleted, or reports the driver of a configured integration as deleted, the UI SHALL remove every loaded entity of that integration from all pages, all groups and the entity store, without waiting for per-entity delete events.

#### Scenario: Integration deleted
- **WHEN** an integration with three entities on the current page is deleted
- **THEN** the three tiles disappear and the entities are no longer known to the UI

### Requirement: Entity availability
An entity SHALL count as available unless its state is Unavailable; an entity without a reported state counts as available, and the core's `enabled` flag has no effect. When the connection to the core is lost every entity SHALL be set to Unavailable; it becomes available again only through a state reported by the core afterwards. An unavailable entity's tile SHALL fade to 50 % opacity within 300 ms and show a `uc:ban` icon in place of its icon, keeping its name; after a connection loss the status line keeps the last reported state text. An unavailable entity's control screen SHALL be covered below its header by a black overlay at 85 % opacity which, after 1 s, shows a red `uc:ban` icon and "Entity unavailable"; the overlay disappears as soon as the entity is available again. Tile and control screen SHALL show a red `uc:link-slash` icon while the entity's integration reports a state other than `connected`.

#### Scenario: Core connection lost
- **WHEN** the socket to the core is lost
- **THEN** all tiles are greyed out with the ban icon

#### Scenario: Entity comes back
- **WHEN** the core reports state `ON` for an unavailable light
- **THEN** the tile returns to full opacity and the control screen overlay disappears

#### Scenario: Integration disconnected
- **WHEN** the integration of a media player reports `disconnected` while the player is still available
- **THEN** the tile and the control screen show the red broken-link icon

### Requirement: Localized entity names and unit system
An entity name SHALL be resolved from its name map in the UI language as described in the `localization` capability; an empty map gives an empty name. When the UI language changes, every entity that has been shown SHALL re-resolve its name immediately and its translated state text 500 ms later; an entity shown for the first time uses the current language. When the unit system changes, every climate entity that has been shown SHALL switch its temperatures to the new unit; a climate entity shown for the first time uses the current unit system.

#### Scenario: Language switched to German
- **WHEN** the language changes to `de_DE` and a light's name map has `en` and `de`
- **THEN** the tile shows the `de` name at once and its state text in German half a second later

### Requirement: Rename an entity
"Rename" in the entity edit menu SHALL open the "Rename entity" dialog prefilled with the current name, with the on-screen keyboard, Cancel and Rename. Rename (or Return in the field) SHALL send `update_entity` with a name map that is the entity's current map with the entry for the current UI language code set to the new name, and close the dialog; the new name appears when the core reports the change. An empty name SHALL show the field error "Input field is empty" with an error haptic for 2 s and send nothing. Cancel, BACK and HOME SHALL close the dialog. A rejected request SHALL show the warning "Error while setting entity name: <message>".

#### Scenario: Rename in English
- **WHEN** the UI language is `en_US`, the name map is `{en: "Lamp", de: "Lampe"}` and the user renames to "Desk lamp"
- **THEN** `update_entity` is sent with `{en: "Lamp", en_US: "Desk lamp", de: "Lampe"}`

#### Scenario: Empty name
- **WHEN** Rename is triggered with an empty field
- **THEN** the field shows "Input field is empty" and no request is sent

### Requirement: Change the icon of an entity
"Change icon" in the entity edit menu SHALL open the icon selector; choosing an icon SHALL send `update_entity` with only the icon. The new icon appears when the core reports the change. A rejected request SHALL show the warning "Error while setting entity icon: <message>".

#### Scenario: Icon selected
- **WHEN** the user picks `uc:lightbulb` for a switch
- **THEN** `update_entity` with icon `uc:lightbulb` and no name is sent

### Requirement: Configured-entity count of an integration
The integration details SHALL show the number of configured entities of the integration, obtained with `get_entities` for that integration with limit 1 and page 1 and taken from the paging count. It SHALL be requested when the details are opened and 500 ms after the entity manager closes. A failed request SHALL only be logged and leave the number unchanged.

#### Scenario: Entities added
- **WHEN** the user adds two entities in the entity manager and it closes
- **THEN** the count is requested again 500 ms later and shows two more

### Requirement: Available and configured entity lists
Entity selection lists SHALL load from the core in pages: configured entities with `get_entities` (filter `integration_ids`, `entity_types`, `text_search`), available entities of an integration with `get_available_entities` (filter `integration_id`, `entity_types`, `text_search`, `entities` = `NEW`, `force_reload` = true), so only entities not yet configured are offered. Opening a list SHALL reset search and type filter and load page 1 with limit 100; the next page SHALL be loaded when the list is flicked to its end or DPAD_DOWN is pressed on the last row, until ceil(count / limit) pages are loaded. Rows SHALL appear in the order the core delivers them, each with icon, name (up to two lines) and entity id; the total count is the core's paging count. While a further page loads, a "Loading" indicator SHALL cover the list for at most 5 s and the list cannot be scrolled. An empty list SHALL show "No entities". Items in these lists are snapshots and do not follow entity change events.

#### Scenario: Integration with 230 new entities
- **WHEN** the available list of an integration opens and the core reports 230 entities
- **THEN** 100 rows are shown, and scrolling to the end loads rows 101–200, then 201–230

#### Scenario: Already configured entity
- **WHEN** an integration offers an entity that is already configured
- **THEN** it does not appear in the available list

### Requirement: Search and type filter in entity lists
Every change of the search text SHALL reload the list from page 1 with the text as `text_search`. The filter button SHALL open a "Filters" sheet with the types Button, Climate, Cover, Light, Media player, Sensor and Switch; toggling a type adds or removes it from `entity_types` and reloads the list from page 1, "Clear" removes all types, "Done", a tap outside, BACK or HOME close the sheet. With the keypad DPAD_UP / DPAD_DOWN move through the types, DPAD_MIDDLE toggles, DPAD_LEFT clears. The filter button SHALL be highlighted while at least one type is selected.

#### Scenario: Filter lights
- **WHEN** the user selects Light and Switch in the filter sheet
- **THEN** the list reloads with `entity_types` [`light`, `switch`] and the filter button is highlighted

#### Scenario: Search
- **WHEN** the user types "kit"
- **THEN** the list is reloaded after every keystroke with the current text

### Requirement: Selecting entities in a list
Tapping a row, or DPAD_MIDDLE on the selected row, SHALL toggle its check mark. "Select all" SHALL check every loaded row and turn into "Clear", which unchecks every loaded row. Select all / Clear and the action button SHALL be dimmed to 30 % and disabled while the list is empty. With the keypad the selection SHALL walk from the filter button through the rows to the footer, where DPAD_LEFT / DPAD_RIGHT choose between Select all / Clear and the action button; an empty list skips the rows.

#### Scenario: Select all with more pages
- **WHEN** 100 of 230 rows are loaded and the user taps Select all
- **THEN** only the 100 loaded rows are checked

### Requirement: Adding and removing configured entities
Adding the selected rows of an integration's available list SHALL send `configure_entities_from_integration` with the integration id and the selected entity ids; removing the selected rows of its configured list SHALL send `delete_entities` with the selected ids, without confirmation. The configured entities appear or disappear through the core's entity events. A rejected request SHALL show the warning "Couldn't configured entity: <message>" or "Couldn't delete entities: <message>". The entity manager screens around these lists are described in the `integrations` capability.

#### Scenario: Remove two entities
- **WHEN** the user checks two configured entities and taps Remove
- **THEN** `delete_entities` is sent with both ids and the entities disappear when the core reports their deletion

#### Scenario: Configure fails
- **WHEN** the core rejects `configure_entities_from_integration` with message "Integration not found"
- **THEN** the warning "Couldn't configured entity: Integration not found" is shown

### Requirement: Adding entities to a page
"Add entity" in the page menu SHALL open the "Add entities" list of all configured entities with a close icon. Add SHALL add the checked entities to the page, clear the selection and close the list. Add without a checked entity SHALL show the notification "Select entities" / "Please select entities to add by tapping in the list." and keep the list open. The close icon, BACK and HOME SHALL close the list without adding.

#### Scenario: Add two entities
- **WHEN** the user checks two entities and taps Add
- **THEN** both are added to the current page and the list closes
