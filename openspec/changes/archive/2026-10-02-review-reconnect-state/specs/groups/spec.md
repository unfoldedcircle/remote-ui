## MODIFIED Requirements

### Requirement: Group definition and loading
A group SHALL be a named, ordered list of entities of one profile with an optional icon, shown as a single tile on a page. Groups of the current profile SHALL be fetched with `get_groups` when the profile is loaded; groups of other profiles are ignored. When group loads overlap, only the answer to the most recent `get_groups` SHALL be applied, and its groups SHALL be taken as groups of the profile that request was sent for. A group that is already known when it arrives again — from the answer after an event announced it, or twice in overlapping loads — SHALL replace the earlier copy, so each group exists once.

#### Scenario: Groups request fails
- **WHEN** `get_groups` fails
- **THEN** no error is shown; group tiles on the pages show no entities

#### Scenario: Duplicate entity
- **WHEN** an entity that is already in a group is added to it
- **THEN** it is not added again and the notification "<entity id> already exists in this group." is shown

#### Scenario: Late answer for the previous profile
- **WHEN** the profile is switched while the groups of the previous profile are still loading, and their answer arrives after the new request was sent
- **THEN** the late answer is dropped and only the groups of the new profile are shown

#### Scenario: Group announced while the groups load
- **WHEN** a NEW group event arrives while `get_groups` is on its way, and the answer contains the same group
- **THEN** the group exists once, with the content of the answer
