## MODIFIED Requirements

### Requirement: Rename an entity
"Rename" in the entity edit menu SHALL open the "Rename entity" dialog prefilled with the current name, with the on-screen keyboard, Cancel and Rename. Rename (or Return in the field) SHALL send `update_entity` with a name map that is the entity's current map with the entry for the current UI language code set to the new name, and close the dialog; the new name appears when the core reports the change. An empty name SHALL show the field error "Input field is empty" with an error haptic for 2 s and send nothing. Cancel, BACK and HOME SHALL close the dialog. A rejected request SHALL show the warning "Error while setting entity name: <message>". A rename for an entity the UI does not hold — one that was never loaded, or one that has been removed while its screen was still open — SHALL send nothing, SHALL leave the app running, and SHALL log the entity id.

#### Scenario: Rename in English
- **WHEN** the UI language is `en_US`, the name map is `{en: "Lamp", de: "Lampe"}` and the user renames to "Desk lamp"
- **THEN** `update_entity` is sent with `{en: "Lamp", en_US: "Desk lamp", de: "Lampe"}`

#### Scenario: Empty name
- **WHEN** Rename is triggered with an empty field
- **THEN** the field shows "Input field is empty" and no request is sent

#### Scenario: Entity deleted while its screen is open
- **WHEN** the entity is deleted elsewhere while its screen is open and the user then confirms a rename for it
- **THEN** no request is sent, the app keeps running and the id is logged as a warning

### Requirement: Change the icon of an entity
"Change icon" in the entity edit menu SHALL open the icon selector; choosing an icon SHALL send `update_entity` with only the icon. The new icon appears when the core reports the change. A rejected request SHALL show the warning "Error while setting entity icon: <message>". An icon change for an entity the UI does not hold SHALL send nothing and SHALL log the entity id, as for a rename.

#### Scenario: Icon selected
- **WHEN** the user picks `uc:lightbulb` for a switch
- **THEN** `update_entity` with icon `uc:lightbulb` and no name is sent

#### Scenario: Icon of an entity that is gone
- **WHEN** an icon is chosen for an entity that is no longer loaded
- **THEN** no request is sent and the id is logged as a warning
