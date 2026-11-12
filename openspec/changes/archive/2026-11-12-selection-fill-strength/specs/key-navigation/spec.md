## ADDED Requirements

### Requirement: Selection fill strength
The selection fill of menu-like lists SHALL take the strength the user picked in "Selection background" (`settings-menu`): five levels from no fill to the strongest fill of the design system, the second highest by default. At the lowest level a fill-style selection SHALL draw no fill. The ring style SHALL not change with the setting.

#### Scenario: Strongest level
- **WHEN** the level is the highest and the d-pad moves to an entity tile
- **THEN** the tile is filled with the strongest fill of the design system

#### Scenario: No fill
- **WHEN** the level is "Off" and the d-pad moves to a row of the Settings menu
- **THEN** the row has no fill and no ring

#### Scenario: Ring unchanged
- **WHEN** the level is "Off" and the d-pad moves to a switch on a setting screen
- **THEN** the switch gets the 3 px ring as before
