## MODIFIED Requirements

### Requirement: Legibility

Font sizes, colours, contrast and the rendering of the d-pad selection SHALL follow the design
system (`docs/design-system.md`), accepted on 2026-10-07. Both hardware models SHALL use the same
palette. Text SHALL be at least 22 px, and text at 22 px SHALL be used only for captions. Text that
carries information SHALL have a contrast of at least 4.5:1 against its background, measured on a
true black for the page background; it SHALL NOT be dimmed with opacity, which is reserved for
disabled controls and dim layers. A change that needs a size, colour or selection look the design
system does not specify SHALL add it to the design system in the same change.

#### Scenario: New screen after acceptance

- **WHEN** a screen is added after the design system has been accepted
- **THEN** its text sizes, colours and selection follow the design system

#### Scenario: Secondary text on the Remote 3

- **WHEN** a help text, a description or a state line is shown on the page background
- **THEN** it is drawn in the secondary text colour with at least 8:1 against black, so it stays
  readable on the Remote 3 LCD in a lit room

#### Scenario: Smallest text

- **WHEN** a screen shows a caption such as a timestamp or a badge
- **THEN** it is at least 22 px; running text, help text and values are larger

#### Scenario: Same palette on both remotes

- **WHEN** the same screen is shown on a Remote Two and a Remote 3
- **THEN** it uses the same colours on both
