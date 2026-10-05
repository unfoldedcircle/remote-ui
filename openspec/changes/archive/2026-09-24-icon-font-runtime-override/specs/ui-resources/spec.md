## MODIFIED Requirements

### Requirement: Embedded assets and fonts
The binary SHALL embed the icon font in its Free edition, its name mapping, its fallback mapping, the dock pictures (Dock 2; Dock 3 dark and silver, each charging and not charging), a small loading image, the quarter-arc image of the startup animation and the keypad picture of the button simulator. Text SHALL use the font families "Poppins" (primary, default 30 px) and "Space Mono" (secondary, default 24 px), each also offered with the first letter of every word or all letters capitalised, and the status bar clock SHALL use Poppins 24 px with 110 % letter spacing. These text fonts SHALL NOT be embedded: they are taken from the fonts installed on the system (the device image, or e.g. `~/.fonts` on a desktop), and Qt's default font is used when they are missing. Custom icons, TV channel icons, background images, sounds, sound effects, legal documents and the licensed icon font SHALL only come from the file system.

#### Scenario: Desktop without Poppins
- **WHEN** the simulator runs on a desktop where Poppins and Space Mono are not installed
- **THEN** all text is drawn in the system's fallback font while icons still render from the embedded icon font

#### Scenario: Startup log
- **WHEN** the icon font loads successfully at startup
- **THEN** "Icon font loaded:", the family the font registered and the path it was loaded from — the file named in `UC_ICON_FONT_PATH` or the embedded resource — are logged

### Requirement: Icon font edition
The icon font SHALL exist in two editions that differ only in which of the mapped icon names they can draw and in the drawing weight: the redistributable **Free** edition, which SHALL be the one kept in the repository and embedded in every binary — every developer build, every unit test, every CI job, every release artefact and every build from the published sources — and the licensed **Pro** edition, which SHALL never be committed to any repository and never be built into a binary; the firmware SHALL install it as a file on the device and name it in `UC_ICON_FONT_PATH`. At start-up the UI SHALL load the file named in `UC_ICON_FONT_PATH` when the variable is set and the file exists, is readable and loads as a font, and the embedded Free edition otherwise, logging a warning with the path when a path was set but could not be used; exactly one icon font SHALL be registered. Both editions SHALL carry the same font family name, so that no part of the UI names an edition. About 1450 of the 3877 mapped icon names SHALL resolve in the Free edition and about 3300 in the Pro edition; a mapped name the loaded edition cannot draw is replaced as specified in "Missing and default icons". The repository check SHALL fail if the icon font kept in the repository is not the Free edition, does not match its recorded provenance, or still carries its original vendor family name.

#### Scenario: Device build
- **WHEN** the UI starts on a device whose firmware installed the licensed edition and set `UC_ICON_FONT_PATH` to it
- **THEN** that file is loaded, the log names its path, every mapped icon name renders as its own icon in the Light weight, and neither a fallback nor the placeholder is ever drawn

#### Scenario: Public build
- **WHEN** the UI starts without `UC_ICON_FONT_PATH` — a desktop build, a unit test, a CI job, a build from the published sources, or a custom build installed by a user, which runs in a sandbox where the firmware's file is not available
- **THEN** the embedded Free edition is used, the icons render in its weight, and the names only the Pro edition has render as their fallback icon or as the placeholder

#### Scenario: Override that cannot be used
- **WHEN** `UC_ICON_FONT_PATH` names a file that does not exist, is not readable or is not a font
- **THEN** a warning naming the path is logged, the embedded Free edition is used, and the UI starts normally

#### Scenario: Licensed font committed by mistake
- **WHEN** the icon font kept in the repository is replaced by the licensed edition
- **THEN** the repository check fails and the change cannot be merged

#### Scenario: New Pro-only icon used in the UI
- **WHEN** a change starts using an icon name that only the Pro edition has and adds no fallback entry for it
- **THEN** a build running without the licensed file draws the placeholder there and logs the missing fallback, which is how the gap is found
