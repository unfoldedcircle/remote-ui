# Remote UI Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## Unreleased
### Added
- The "Join other network" dialog and the WiFi password dialog can be filled with the d-pad: the SSID field, the
  hidden network option, the security options and Cancel / Next / Join are walked in order, Return on a field submits
  it and the on-screen keyboard follows the fields. This covers the WiFi settings and the WiFi page of the dock setup.
- Pages can be managed with the d-pad: the page menu (long press on HOME) opens the page selector, a long press on
  OK there enters the edit mode, where OK renames a page, a long press picks it up to move it with UP / DOWN, RIGHT
  reveals its delete and DOWN past the last page reaches the "+" for a new page. In the page's own reorder mode OK
  picks the selected tile up, UP / DOWN move it and BACK saves the order.
- The profile list reaches the "+" for a new profile with DOWN, and a long press on OK opens a profile's menu
  (rename, icon, delete). The rows above the settings menu on the profile page (profile, web configurator switch,
  address, PIN, QR code) are reached with UP from the first menu entry. The icon selection walks its grid with the
  d-pad, reaches the tabs with UP and Close with DOWN. The Back button of a failed software update is reachable.
- The integration and dock setup forms can be filled with the d-pad: text fields, checkboxes and dropdowns are walked
  in order, Return on a field moves to the next one, the on-screen keyboard follows the field and gets out of the way
  of the buttons, a dropdown opens the same selection list as the other settings, and the finish screen, the
  add-entities step, Cancel and Next are all reachable. BACK cancels the setup that is running instead of only
  closing its popup.
- Entity lists (adding entities to a page or a group, managing the entities of an integration) are walked with the
  d-pad throughout: the rows, the filter button, Select all / Clear and Add / Remove. Groups can be created and
  edited with the d-pad, including reordering their entities (OK picks a row up, UP / DOWN move it, OK drops it) and
  removing one (long press on OK). The page menu (long press on HOME) offers the edit menu of the selected tile, so
  rename, change icon, remove and the group editor are reachable without the touch screen.
- The docks and integrations settings can be worked with the d-pad throughout: the "Add a new dock" and
  "Add an integration" sheets, the discovery inside them, the dock and integration details with every action
  in them (rename, identify, connect, LED brightness, password, factory reset, delete), the entity filter of
  the entity lists, and the add-entities step of an integration setup. The delete drawer of a dock or
  integration starts on Cancel so that deleting takes a deliberate second step.

### Changed
- The readiness check of an activity now has its own screens instead of a generic notification. The first screen
  names the activity, says how many devices need attention and at which step the activity would stop; "What is
  wrong" opens the predicted run step by step, with every device that will not react, the steps that are skipped
  or not needed, and a mark where the run stops. Cancel and Proceed work from both screens, and the whole flow
  can be operated with the keypad.
- A failed activity or macro run now says what happened at a readable size: the name of the activity, the step it
  stopped at, the device that did not react and the reason reported by the remote, followed by Close and Try again.
  Try again repeats the same command in the same direction right away, without another readiness check. Tapping
  anywhere still closes the screen, and both buttons work with the d-pad.

### Fixed
- The WiFi settings could not be walked with the d-pad past the WiFi band row: the known and other network
  lists were stepped over, and the "Join other" button had no way to be selected. The lists now move their
  own selection, and a scan result no longer rebuilds them every few seconds — a rebuild threw the selection
  and the focus back to the top of the page. Saved and scanned networks are updated in place and listed
  strongest signal first.
- Selecting a settings row with the d-pad now brings the whole row on screen, title and description included,
  instead of only the switch. At the end of a page DOWN scrolls on to the remaining content, and UP at the
  top scrolls back. The value labels below the last slider of the power settings were cut off even by touch.
- OK on the dock rename and password dialogs, the profile name, entity rename and group rename dialogs sent
  the request twice: once for the d-pad handler and once for the input field's Return.
- BACK in the profile rename, page add and page rename dialogs closed the list behind the dialog instead of
  the dialog; the dialogs now own the input, and their Cancel / OK buttons are reachable with the d-pad.
- The entity filter sheet and the add-entities step of an integration setup could not be left with the
  keypad. HOME closes the profile PIN prompt.
- BACK on a running dock or integration setup now cancels the setup instead of only closing the popup.
- Closing a selection list in the localisation settings with BACK left its selection handler active, so the
  next selection in any other list of that page was written into every setting whose list had been cancelled
  before, e.g. a timezone id into the language, country and unit system. Core rejected these with "Invalid
  language code format" and "unknown variant ``" errors, which also showed up as seemingly random errors while
  navigating the language, country, timezone and unit system lists. The settings now refuse empty or unknown
  values instead of sending them to core, and a localization update from core that lacks a field no longer
  clears the cached value that the next change would send back empty.
- An activity that failed or timed out while its screen was open never showed it. The screen header was meant to turn
  red in that case, but the condition behind it asked for the activity to be in two states at once and could never be
  true, so the colour had never appeared since it was written.
- The "Tap for more" hint under the activity name was nearly invisible on that red header: its dim grey is made for
  the black background. It is shown in the regular light text colour while the header is red.

---

## v0.81.1 - 2026-09-01
### Changed
- The check that runs before an activity is switched on or off now asks the remote for its full readiness report
  instead of guessing from the list of devices in the sequence. The message names what is actually wrong — a
  disconnected integration, an unavailable dock or IR emitter, Bluetooth being off, a device deleted after the
  activity was set up — with the device names in the display language, and lists one line per problem instead of one
  per affected device. Activities that only look unready are no longer reported: a device that is already in the
  wanted state, and infrared or macro steps that have no integration to check, are now understood by the check.

## v0.81.0 - 2026-09-01
### Added
- The initial setup suggests countries where the chosen language is spoken: after picking e.g. "Deutsch", Germany,
  Austria, Switzerland and the other German-speaking countries appear at the top of the country list, with the most
  likely one preselected. All other countries follow alphabetically, and searching by name or two-letter country code
  still finds any of them.
- Countries with a single timezone — about two thirds of all countries — no longer show a timezone list at all during
  setup. A confirmation card shows the derived timezone ("Switzerland — Zurich · GMT+01:00") and one press continues.
  Countries with several timezones show only their own zones, labelled with the city and GMT offset instead of raw
  identifiers like "America/New_York", sorted east to west. An "All timezones…" entry keeps every zone reachable for
  special cases, during setup and in the localisation settings.
- The language list opens with the current language preselected, in the setup and when returning to it.

### Fixed
- The timezone list offered during setup and in the localisation settings contained more than a hundred unrelated
  entries: every timezone in the world that happened to share the UTC offset of the selected country was listed, and
  the list even changed with daylight saving time. Only the timezones of the selected country are listed now.
- Country names in the country list were often shown in the wrong language. They now follow the selected UI language,
  with English as the fallback.

## v0.80.0 - 2026-08-30
### Fixed
- Remote entities did not pick up the `send_cmd` and `toggle` features reported by an integration, and remote and
  activity entities did not pick up `on_off`. The features were discarded as unsupported.
- Voice assistant entities did not pick up the `transcription` feature, so the spoken command was never shown as text.
- Binary sensors with the `garage_door` device class fell back to a generic on/off sensor instead of showing
  "Opened" / "Closed".
- Activities containing infrared remotes, IR emitters or macros in their start or stop sequence always stopped with
  "Some devices are not ready" since v0.76.0. Those entities belong to the remote itself and have no integration
  that could be connected; they are left out of the readiness check again, as before v0.76.0.


## v0.79.0 - 2026-08-27
### Added
- "Fix states" in the activity menu is now working: tapping an entity lets you record that the device is on or off,
  without sending any command to it. Use it when the remote's idea of a device's state drifted from reality, e.g. an
  IR-controlled device that was switched with its original remote, so that the next activity powers it on or off
  correctly. Only entities with an on/off state are listed (switches, lights, climates, media players, remotes and
  activities); sensors, buttons, macros and the like are left out. Requires a remote-core version with the
  `set_entity_state` API
- The activity menu ("Tap for more") can be used with the d-pad: up / down move over "Fix states" and the included
  entities, the middle button opens the selected entry, BACK returns from the "Fix states" list to the menu and
  closes the menu from there

### Fixed
- The initial setup screens could not be operated with the d-pad: the terms & conditions, WiFi network, dock discovery
  and results, integration and final screens only reacted to touch. Every setup step now navigates with the d-pad and
  selects with the OK button, the same way the settings pages do, and BACK returns to the previous step.
- The final "You're all set" setup screen could not be scrolled, so in languages with a longer description than English
  the "Done" button was pushed off the display and could not be reached. The screen now scrolls.
- The keypad selection outline is only shown once the d-pad is used and disappears again as soon as the screen is
  touched, so a screen opened by touch no longer starts with a highlighted control or PIN digit.
- The buttons of the "Profile already exists" prompt during the initial setup overlapped each other in languages
  with longer labels.
- The "Useful tips" shown after the initial setup were English-only images; they are now regular, translatable
  screens that cover the whole display instead of leaving a strip of the main screen visible below them.
- The d-pad LEFT/RIGHT keys switched the pages behind the tips when the tips were shown right after the initial
  setup; they now only move between the tips.
- Joining a WiFi network during the initial setup always ended with "Failed to connect", even with the correct
  password, while the very same network could be joined from Settings afterwards. The setup screen gave up after three
  seconds and deleted the network it was in the middle of connecting to. It now waits for the remote to report the
  result and only treats the join as failed on a wrong password, a network that cannot be found, or after a realistic
  timeout
- The administrator PIN keypad in the onboarding drew a permanent outline around the "1" key that could not be moved
  or dismissed, and did not react to the d-pad at all. The outline is now only shown once the d-pad is used, moves
  with up / down / left / right, and the middle button enters the outlined digit. Entering the PIN by touch shows no
  outline. This applies to every PIN keypad: the onboarding, the "Administrator PIN" settings screen, and the
  administrator PIN prompt when switching to a profile from a restricted one, which did not react to the d-pad
  before and can now also be cancelled with the BACK button
- The BACK button returns to the previous step from the administrator PIN step of the onboarding, like on the other
  steps

## v0.78.0 - 2026-08-25
### Added
- Full translation texts for every language, and Finnish as a new language.
### Fixed
- The on-screen keyboard closed after every single letter typed into the search field of a media player's browse
  screen, so a search term had to be entered one letter at a time, tapping the search field again in between. The
  keyboard now stays open until the search is submitted with its "Search" key, the back button is pressed, or the
  result list is tapped
- Searching a media player no longer brings up the full-screen loading indicator, which blocked the whole screen —
  including the keyboard — whenever a search took longer than half a second. Searching now shows a small indicator
  above the results instead
- A media search that failed, timed out, or was sent while the remote was not connected to the core left the
  loading indicator spinning and the remote unresponsive for minutes. Such a search now reports the failure right
  away, and its "Retry" option repeats the search instead of reloading the browse screen
- A media search error other than "not found" or a timeout closed the whole browse screen, discarding the search
  term. The screen now stays open so the term can be corrected
- Results of a slower earlier search could replace the results of the term that was actually typed
- Clearing the search field with its ✕ button closed the keyboard as well
- The up, down and middle keypad buttons moved and opened entries in the result list hidden behind the keyboard.
  While the keyboard is up the middle button now runs the search, and up and down do nothing
- The d-pad acted on the screen behind a popup as well as on the popup itself: moving up and down in a selection list,
  for example the WiFi band or any of the Localisation settings, also moved the selection on the page underneath, and
  the middle button could reopen the list that was already open
- Leaving the release notes of a software update left the screen without a selection, so the d-pad did nothing until
  the settings were closed and opened again. Coming back from any settings screen now returns to the entry the screen
  was left from
- Holding the d-pad down on the Localisation settings did not move the selection until the button was released, and a
  single press only took effect a moment later
- The Regulatory, Terms, Warranty and Licenses screens scrolled their text off the top when it was shorter than the
  screen
- The Microphone and Speech response switches in the voice control settings were missing the check mark that every
  other switch shows when it is turned on

## v0.77.0 - 2026-08-17
### Added
- New user interface setting "Open activities started with the API": when an activity is turned on from outside the
  remote, its screen is opened right away, replacing whatever is currently on screen. Activities started on the remote
  keep opening their screen as before
### Fixed
- A button pressed while the remote is asleep reported "device is not responding" the moment the remote woke up.
  Such a command is now sent again for the whole interval configured under Power, and only reported as failed if it
  has not gone through by the end of it. This also covers the press that wakes the remote in the first place, which
  happens before the remote knows it is awake again and was therefore never retried
- A command sent while the remote is not connected to the core disappeared without any reaction at all: no retry, no
  error and no loading indicator
- The loading indicator could keep spinning forever after an entity was deleted or the connection to the core was
  lost, and sending the same command again was refused while it did
- Holding down a button on an unreachable device queued up one "device is not responding" prompt per repeat, and
  offered to send button presses again long after they were released
- Retrying a command with the middle d-pad button left the notification behind, which then silently suppressed every
  later message about the same device
- Starting an activity whose start sequence fails could spin forever without ever showing an error. This happened
  with a sequence of a single command, for example one infrared command sent to a dock that is not connected:
  starting such an activity again after it had already failed never reported the second failure
- The activity start screen now names the reason a step failed, for example "Connection to dock not established",
  instead of only stating that something went wrong
- The activity start screen kept spinning when the entity became unavailable mid-sequence, for instance after the
  connection to the core was lost
- Starting or stopping an activity with the button press that wakes the remote asked "Some devices are not ready"
  straight away. The readiness check now waits for the devices of the activity to come back for the whole interval
  configured under Power, the same budget a single command gets, and only asks once that interval has passed
- The readiness check reported an activity as ready when neither its devices nor their integrations were known to the
  remote yet, which is exactly the case right after a wakeup, and started the activity into devices that were all
  still disconnected
- Pressing an activity, or the power button, several times while its devices were still coming back after a wakeup
  ran the sequence once per press
- Turning an activity off from its own screen while its devices were not ready left the activity screen open after
  choosing Proceed
- A power button press made on a sleeping remote was dropped when the wakeup took longer than two seconds, even with
  a longer interval configured under Power

---

## v0.76.0 - 2026-08-12
### Added
- Hidden network option when joining a WiFi network by entering its name
- WiFi security can be chosen when joining a network by entering its name: None, Auto, WPA/WPA2 Personal,
  WPA2/WPA3 Personal and, on the Remote 3, WPA3 Personal. Auto leaves the choice to the remote and is preselected
- Switches, checkboxes and buttons in the settings can now be operated with the middle d-pad button
- WiFi networks can be selected and joined with the d-pad, including the "Join other" and "Delete all networks" buttons
- The administrator PIN can be entered with the d-pad
- The "Add a new dock" and "Add an integration" panels can be opened with the d-pad
- The dock and integration detail screens can be scrolled with the d-pad

### Fixed
- The release notes of a software update could not be closed with the back button
- Settings screens now scroll along when the d-pad moves the selection below the visible area
- The Voice Control settings did not react to the d-pad at all
- On the WiFi settings screen the d-pad stopped at "Active WiFi scanning" and could not reach the rest of the screen
- The 24-hour time setting could not be changed with the d-pad
- On the Software update screen a single d-pad press both scrolled the screen and moved the selection
- The About screen could not be scrolled, hiding entries on smaller screens
- Pressing the middle d-pad button on an empty dock or integration list did nothing instead of failing silently
- Settings screens occasionally opened with nothing selected, leaving the d-pad without effect
- Known WiFi networks always showed a full signal strength instead of the measured one
- A WiFi network reachable through several access points showed the signal strength of an arbitrary one instead of
  the strongest, and every additional access point kept memory allocated until the remote was restarted
- Joining a WiFi network from the list could configure the wrong network, or fail, if its name contains characters
  the remote cannot display. Two networks in range sharing such a displayed name were also shown as one entry

## v0.75.0 - 2026-08-07
### Added
- Portuguese translation

### Fixed
- The Spanish, Norwegian, Polish and Swedish translations could not be selected

---
## v0.74.5 - 2026-07-28
### Breaking Changes
- The Git history of this repository was rewritten to remove a file that may not be redistributed.
  Commits from v0.62.1 to v0.74.5 have new commit IDs.
  Older commits, all tags and all releases are unchanged, and the application itself is not affected by the rewrite.
  An existing clone cannot be updated with `git pull` and has to be reset.

  <details>

  <summary>Git CLI instructions</summary>

  ```shell
  # 1. Check that "origin" is this repository. If you cloned your own fork, "origin" is that fork,
  #    which is not rewritten: use the remote that points here (usually "upstream") in the commands
  #    below, and reset your fork afterwards in the same way.
  git remote -v

  # 2. For each branch of your own, note where it branched off main, while main is still the old one.
  git merge-base main <your-branch>

  # 3. Replace main with the rewritten history. Do not use git pull or git merge here: the old and
  #    the new history are unrelated, and merging them brings the removed file back.
  git fetch origin
  git switch main
  git reset --hard origin/main

  # 4. Move your own commits onto the new main, using the commit noted in step 2.
  git rebase --onto main <commit-from-step-2> <your-branch>

  # 5. Remove the old commits, and the removed file with them, from your clone.
  git reflog expire --expire=now --all
  git gc --prune=now
  ```

  </details>

### Added
- Spanish, Norwegian, Polish and Swedish translations

---
## v0.74.4 - 2026-07-27
### Fixed
- The touch slider did nothing when an activity mapped it to a light or a cover without picking a feature explicitly

---
## v0.74.3 - 2026-07-27
### Fixed
- The home screen jumped into the activity header area when the header resized, for example when starting or stopping an activity

---
## v0.74.2 - 2026-07-25
### Fixed
- Touch slider not working for lights in acitivites
- Page incorreclty jumps to top over boundaries

---
## v0.74.1 - 2026-07-22
### Added
- A loading indicator on the home screen and on the entity while a command is running or being retried in the background after waking from suspend

### Fixed
- The settings menu could not be scrolled when it had more entries than fit on screen
- Could not join a WiFi network during onboarding because a background scan removed the selected network before the join completed

---
## v0.74.0 - 2026-07-21
### Added
- A Touch Slider settings screen to adjust the slider sensitivity per use case (volume, brightness, cover position, seek) with a live test area
- Smoother and more precise touch slider tracking that follows your finger movement

### Fixed
- The touch slider did not control brightness for dimmable lights
- The touch slider could silently stop working until the volume was changed with the buttons
- The touch slider could stop responding after the remote woke from suspend
- Volume kept adjusting after releasing the button when controlling a device via IR
- The dock detail view showed the wrong dock image
- Starting an activity could show an error that repeats the same device name over and over
- The media widget in the activity header sometimes stayed empty
- Some media player features were missing or not recognized
- Shuffle and repeat did not work on speakers
- Some media types were not recognized and media browsing could fail to open items with special characters in their names
- Profile pages could fail to load
- Button presses were sometimes handled incorrectly, including power off on long press
- The web configurator PIN could show up with fewer than 4 digits
- The remote could slow down over long use (memory is now released properly after Wi-Fi scanning and voice playback)
- Voice control could stop responding after audio playback
- Devices could show outdated information after the remote reconnected or an integration was reconfigured
- General stability improvements

### Changed
- All devices are now loaded when the remote starts, so pages and widgets are ready right away
- Smoother scrolling and a more responsive interface, especially while media is playing
- Album art loads faster
- Faster and more responsive touch slider
- Passwords, PINs and tokens are no longer written to the device logs
- New translation platform (SimpleLocalize) for community translations

---
## v0.73.5 - 2026-06-05
### Changed
- Standby state change detection for command retry logic


---
## v0.73.4 - 2026-05-08
### Fixed
- Entities not available for media widget and therefore not showing image

---
## v0.73.2 - 2026-05-07
### Fixed
- Prevent stale entity command retries from stopping early

---
## v0.73.1 - 2026-04-28
### Fixed
- Color wheel sending invalid hue values and the selector escaping the circle on fast drags
- Climate detail view showing the temperature without the unit while the main view showed it with the unit
- Entity widgets (notably the media widget) showing stale data after the remote wakes from sleep

### Changed
- Updated translations from Crowdin

---
## v0.72.0 - 2026-04-13
### Added
- Media browser and playback controls in the media widget
- Option to show the battery indicator everywhere

### Fixed
- Sensor and select widget clipping
- Media browsing pagination stopping early
- Retry handling when media browsing fails

---
## v0.71.1 - 2026-03-19
### Fixed
- Adjust color contrast
- Show play indication
- Punch volume and play button through in media browsing
- Search media class filter
- Close media browser after starting

---
## v0.71.0 - 2026-03-18
### Added
- Download progress for software updates
- Media browsing and search
- Coverflow view mode for media browsing
- Option to set coverflow as the default view

---
## v0.70.1 - 2026-02-24
### Added
- Log messages for software update process

---
## v0.70.0 - 2026-02-16
### Added
- Show warning when activity ready check is disabled

### Fixed
- Only check entities in activity on and off sequences

---
## v0.69.0 - 2026-01-22
### Added
- Select entity and select widget support

### Fixed
- Long entity names on activity loading screens won't break into multiple lines
- Missing retry logic from activity power button mapping

---
## v0.68.5 - 2026-01-15
### Fixed
- Entity state check before starting/stopping activities

---
## v0.68.4 - 2026-01-14
### Fixed
- Button control not working when entity opened from an activity
- Activity page indicator visible when activity in header is disabled

---
## v0.68.3 - 2026-01-13
### Fixed
- Dropdown menu button control. Mainly present in activity included entities screen letting button presses through.

---
## v0.68.2 - 2026-01-12
### Fixed
- Long press timer key tracking

---
## v0.68.1 - 2026-01-11
### Fixed
- Media image not shown on page entity

---
## v0.68.0 - 2026-01-09
### Fixed
- Button navigation sproadically stops working
- Voice assistant listening animation still showed after error
- Activity start screen with 0 included entities
- Activity error handling for sequences
- Entity name missing when starting activity for first time
- Text cut off on activity loading screen
- Ignore button presses for unavailable entities

---
## v0.67.0 - 2025-12-24
### Fixed
- Touch slider warning if entity is unavailable
- Sensor widget shows wrong values in activity UI

---
## v0.66.0 - 2025-12-19
### Fixed
- Same sensor value shown for all sensors
- Customer sensor label shows "Custom"
- Voice UI lets button presses through
- Processing showed after voice assistant finished event
- Activity sequence timeout handling
- Activity's entities readyness check
- Invalid media player state
- Media image not shown on main page entities
- Touch slider commands even when entity is unavailable

### Changed
- Improved voice assistant error handling
- Improved activity turn on/off after resume from system sleep

---
## v0.65.10 - 2025-12-10
### Fixed
- Charging screen shown after reboot

---
## v0.65.2 - 2025-12-05
### Added
- Voice Assistant support
- Command retry after wakeup. Wakeup window is configurable in Power Saving settings.

### Fixed
- Media image download timeout handling

### Changed
- Disable certificate validation for media image download

---
## v0.64.4 - 2025-11-27
### Fixed
- Media image not loaded sproadically
- Sensor value not shown within activity

---
## v0.64.3 - 2025-11-23
### Changed
- Display brightness minimum value to 5%


---
## v0.64.1 - 2025-11-21
### Fixed
- Only load media image when it has changed

---
## v0.64.0 - 2025-11-18
### Added
- Touch slider configuration support

---
## v0.63.0 - 2025-11-06
### Added
- Sensor widget support for activities
- Notify before starting an activity if an integration is not ready
- Notify with option to try again if command fails due to device not being ready

### Changed
- Charging screen shows up when a power supply is detected with additional information if the device is charging or just being supplied with power
- Show IP address instead of hostname by default for the Web Configurator

### Fixed
- Sensor UI screens
- Popup menu button handling
- Visilbity of software update icon in the status bar
- Failed marco sequences not shown
- Wifi network list empty during dock setup
- Popup menu trims text for long text items

---
## v0.62.2 - 2025-09-26
### Fixed
- Popup menu button handling

---
## v0.62.0 - 2025-09-23
### Changed
- Reload entity data when entering UI screen
- Update method for loading button mapping

---
## v0.61.0 - 2025-09-22
### Changed
- Starting an activity from another activity will open the new activity's UI

### Fixed
- Rendering of icons
- Show loading icon next to WiFi networks, when connecting

---
## v0.60.1 - 2025-09-19
### Added
- Binary sensor support

### Fixed
- Known WiFi network did not connect when selected

### Changed
- WiFi settings menu

---
## v0.59.0 - 2025-09-12
### Fixed
- Repeat command handling, do not wait for ack

### Changed
- Repeat count increased to 4

---
## v0.58.3 - 2025-08-27
### Fixed
- QR code in pull-down menu and during onboarding
- Popup menu closed when home button released when it has opened

---
## v0.58.2 - 2025-08-26
### Fixed
- QR code in pull-down menu and during onboarding
- Popup menu closed when home button released when it has opened

---
## v0.58.0 - 2025-08-25
### Fixed
- QR code in pull-down menu and during onboarding
- Popup menu closed when home button released when it has opened

---
## v0.57.0 - 2025-08-18
### Fixed
- Language text logic
- High power consumption when display is off

### Changed
- Renamed media image fill option

---
## v0.56.4 - 2025-08-05
### Fixed
- High CPU consumption in low power mode

---
## v0.56.3 - 2025-08-03
### Fixed
- Wifi scan interval slider range

---
## v0.56.2 - 2025-08-02
### Fixed
- High CPU consumption while loading animation is running

---
## v0.56.0 - 2025-07-24
### Added
- WiFi band selection
- WiFi scan interval config option

---
## v0.55.1 - 2025-07-04
### Fixed
- Incorrect dock image shown
- Dock discovery help text

---
## v0.54.10 - 2025-06-06
### Fixed
- Bug in repeat logic

---
## v0.54.9 - 2025-05-27
### Fixed
- Wifi icon size in known networks
- Transparent media image when no media text is shown
- Media player screen shuffle, repeat and app icons cut off
- Activity bar height jumps when image changes
- Media image sometimes not shown
- Touch slider not working with certain device classes

---
## v0.54.5 - 2025-05-23
### Fixed
- Turn off menu only shows entities with on/off features available

## v0.54.4 - 2025-05-19
### Changed
- Media type is displayed as string

### Fixed
- Record, Stop and Menu buttons not working on Remote 3
- Icon shown under transparent media image

## v0.54.2 - 2025-05-12
### Fixed
- Icon shown under transparent media image

## v0.53.2 - 2025-04-06
### Added
- Option to fill available space for media player widget. Can be turned on in Settings / User interface.

### Fixed
- Activity list image and icon sizes
- Media player widget shrinking

## v0.50.2 - 2025-04-03
### Fixed
- Missing media player icon map

## v0.50.0 - 2025-03-31
### Added
- Support for touch slider
- Access profiles, Web Configurator and settings by pulling down the page

### Changed
- Activity bar moved to the page header with option to turn it off in Settings / User Interface

### Fixed
- Missing icons during dock discovery
- Wrong remote name for Remote 3 during onboarding

## v0.49.0 - 2024-02-11
### Added
- Option to show media widget as horizontal

## v0.48.0 - 2024-01-17
### Fixed
- DPAD middle button behaviour on pages
- Sizing of media player widget on activity UI pages. Very small media widget won't show progress bar and media information.
