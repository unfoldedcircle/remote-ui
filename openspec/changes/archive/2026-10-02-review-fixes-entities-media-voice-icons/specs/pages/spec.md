## MODIFIED Requirements

### Requirement: Pages belong to the current profile
The UI SHALL load the pages of the current profile with `get_pages` after the profile has been loaded, in the order returned by the core. Each page has an id, a name, an optional background image and an ordered list of items, each item being either an entity or a group. An item id that appears twice on a page is added only once. When page loads overlap — a reload after a reconnect during a profile switch, or a resync after a rejected page change — only the answer to the most recent `get_pages` SHALL be applied; the answer, success or failure, to an earlier request SHALL be ignored. The applied answer SHALL replace the page list as a whole, including any page an event announced while the request was on its way.

#### Scenario: Pages loaded
- **WHEN** the core answers `get_pages`
- **THEN** all pages are shown as swipeable screens, and the activity bar of every page is computed from the running activities and playing media players that are on that page, directly or in one of its groups

#### Scenario: Pages fail to load
- **WHEN** `get_pages` fails
- **THEN** the page area is shown empty (no-page screen) without an error notification

#### Scenario: Resync after a failed change
- **WHEN** adding, renaming or updating a page is rejected by the core
- **THEN** a warning notification with the core's message is shown and all pages of the profile are reloaded from the core

#### Scenario: Overlapping page loads
- **WHEN** a second `get_pages` is sent before the first one is answered
- **THEN** only the answer to the second request is shown; the answer to the first is dropped, so no page appears twice and no page of another profile is shown

#### Scenario: Page announced while the pages load
- **WHEN** a NEW page event arrives for the current profile while `get_pages` is on its way, and the answer contains the same page
- **THEN** the page is shown once

### Requirement: Page header tap and activity bar
Tapping the page header SHALL open the page selector (or leave the reorder mode when it is on). When the "Activities on pages" setting is on and at least one activity or media player on the page is running, the header SHALL grow to 440 px (680 px when the current activity shows a media image) and show an activity bar: "<name> is <state>", the entity icon or the media widget, and page dots when more than one activity is running. The bar is hidden in reorder mode and when the setting is off. The activity bar of a page SHALL show exactly the running activities and playing media players that are on the page, directly as a tile or as a member of one of the page's groups, and SHALL be recomputed for every page whenever either side changes: an activity or media player starts or stops, the pages are loaded, a page is added or changed, a group is loaded, added or changed, an entity is removed from a group, or an entity, a group or an integration is deleted. An entry that stays in the bar SHALL keep its position; new entries are added after it in the order their activities started.

#### Scenario: Activity bar keys
- **WHEN** VOLUME_UP / VOLUME_DOWN / MUTE / PLAY / PREV / NEXT is pressed on the main screen
- **THEN** for an activity the command of its button mapping (short press) is sent; for a media player the matching player command is sent and the volume overlay is shown for volume keys

#### Scenario: POWER on the main screen
- **WHEN** POWER is pressed and at least one activity is running
- **THEN** a "Turn off" menu lists every running activity (and media player with power); with more than one entry a "Turn off all" entry is added; activities go through the readiness check before being turned off

#### Scenario: Header resizes while the page is at the top
- **WHEN** the header height changes (activity started or stopped, media image appears) and the page is scrolled to its top and no finger is on the screen
- **THEN** the page stays anchored to the top of the header during the 500 ms animation instead of scrolling into the header area

#### Scenario: Activity bar entry tapped
- **WHEN** an activity bar entry is tapped
- **THEN** the screen of that activity or media player opens

#### Scenario: Running activity removed from a page
- **WHEN** a running activity is removed from a page in the web configurator, directly or by removing it from a group on that page
- **THEN** it disappears from that page's activity bar right away, while it keeps running

#### Scenario: Running activity added to a page
- **WHEN** a running activity or a playing media player is added to a page, directly, through a group on the page, or with a new page
- **THEN** it appears in that page's activity bar right away, without the activity being stopped and started again

#### Scenario: Group or entity deleted
- **WHEN** a group holding a running activity, or the running activity's entity or integration, is deleted
- **THEN** the activity no longer appears in the activity bar of the pages that showed it through it
