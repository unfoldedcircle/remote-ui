## MODIFIED Requirements

### Requirement: Search fields keep the keyboard open
A search field (selection lists, entity lists, media browser) SHALL show a magnifier icon and its placeholder while unfocused and empty, and hide both while focused. While the keyboard is up, a search field SHALL keep the focus even when the list underneath is rebuilt by the search, so the keyboard stays open while typing. A ✕ SHALL be shown while the field contains text; tapping it SHALL clear the text and keep the field focused and the keyboard open. In selection lists and entity lists, while the keyboard is up and the search field has the focus, the next tap on the popup below its title bar (the list and the field included) SHALL only hide the keyboard, without acting on what was tapped; in entity lists the first physical button press SHALL also hide the keyboard. Once the keyboard is down, the next tap SHALL act on what was tapped — also when the keyboard was closed with its own hide key rather than by that tap. In the media browser a tap below the search field SHALL hide the keyboard (see `media-player`).

#### Scenario: Live filtering
- **WHEN** the user types "Deu" into the search field of the language list
- **THEN** the list is filtered after each letter and the keyboard stays open

#### Scenario: Clearing the media search term
- **WHEN** the user taps the ✕ of the media browser search field while the keyboard is up
- **THEN** the text is cleared and the keyboard stays open for the next word

#### Scenario: Tap on the filtered list
- **WHEN** the search field of a selection list is focused and the user taps a list entry
- **THEN** the keyboard is hidden and the entry is not selected by that tap

#### Scenario: Tap after the keyboard was hidden with its hide key
- **WHEN** the user taps the search field of the language list, closes the keyboard with the keyboard's hide key and then taps a list entry
- **THEN** that first tap selects the entry
