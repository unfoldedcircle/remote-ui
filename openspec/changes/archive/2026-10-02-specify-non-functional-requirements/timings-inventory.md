# Timings and timeouts found in the code

Inventory taken from `src/` on 2026-09-19. Values are milliseconds unless stated.
Animation and transition durations are listed separately from functional timeouts.

Scope: `src/**/*.cpp`, `src/**/*.h`, `src/qml/**/*.qml`. Excluded: `src/qml/keyboard/layouts/**`,
`3rd-party/`, `tests.bak/`. `Qt.callLater` is not counted as a timing.

## 1. Core connection and requests

| Value | What it controls | Where |
|---|---|---|
| 10000 ms (default, `UC_UI_REQUEST_TIMEOUT`) | how long any request to the core may take before the UI gives up on it and reports "Request timed out" (408) | `src/core/core.cpp:14`, `src/core/core.cpp:1850` |
| 2000 ms | how often the remote retries the websocket connection to the core after it drops | `src/core/core.h:548`, `src/core/core.cpp:34` |
| 10 attempts (~20 s) | how long reconnecting stays silent before the "Connection error" warning appears | `src/core/core.cpp:1402` |
| 60000 ms | keep-alive ping to the core — **dead code**, the timer is never created or started | `src/core/core.h:545`, `src/core/core.cpp:26-28` |
| 2000 ms | retry after the configuration could not be loaded from the core | `src/config/config.cpp:969` |
| 2000 ms | retry after the profile list could not be loaded from the core | `src/ui/uiController.cpp:241` |
| 1000 ms | clock update in the status bar | `src/ui/uiController.cpp:170` |

## 2. Entity commands and retries

| Value | What it controls | Where |
|---|---|---|
| 200 ms | delay before a command that is still running shows the spinner, so quick commands don't flash | `src/ui/entity/entityController.cpp:777`, `:918` |
| 500 ms | pause between two attempts to resend a command issued around a wake-up | `src/ui/entity/entityController.cpp:779`, `:976` |
| 2000 ms (2 s default, user-settable 0–10 s) | how long commands keep being retried after the remote wakes up | `src/ui/entity/entityController.h:254`, `src/ui/entity/entityController.cpp:1132` |
| 4000 ms | how long the activity readiness check may take before the sequence starts anyway | `src/ui/entity/entityController.cpp:24`, `:380` |
| 1000 ms | pause between two readiness re-checks while waiting for devices to come back after a wake-up | `src/qml/main.qml:228` |
| 100 ms | grace period before a removed entity object is destroyed, so the screen showing it can unload | `src/ui/entity/entityController.cpp:746` |
| 500 ms | delay before entity state texts are re-translated after the language changed (12 occurrences: `light.cpp:186`, `switch.cpp:102`, `remote.cpp:119`, `macro.cpp:122`, `cover.cpp:164`, `button.cpp:91`, `activity.cpp:290`, `select.cpp:138`, `climate.cpp:303`, `sensor.cpp:150`, `voiceAssistant.cpp:133`, `mediaPlayer.cpp:1031`) | `src/ui/entity/light.cpp:186` and 11 more |
| 1000 ms | media player playback position counts up while playing | `src/ui/entity/mediaPlayer.cpp:202` |
| max(2000 ms, resume window) | how long a power press made on a sleeping remote stays armed while the wake-up is reported | `src/qml/components/entities/activity/deviceclass/Activity.qml:372` |
| 500 ms | debounce before a changed target temperature is sent | `src/qml/components/entities/climate/deviceclass/Climate.qml:138` |
| 500 ms | debounce before a changed brightness / colour temperature is sent | `src/qml/components/entities/light/Brightness.qml:79`, `:87` |
| 500 ms | debounce before a changed brightness is sent from the colour page | `src/qml/components/entities/light/Color.qml:46` |
| 500 ms | debounce before a changed cover position is sent (4 device classes) | `src/qml/components/entities/cover/deviceclass/Window.qml:127`, `Garage.qml:127`, `Curtain.qml:127`, `Blind.qml:127` |
| 300 ms | how long the colour wheel ignores incoming state after the user moved the picker | `src/qml/components/entities/light/Color.qml:120` |
| 200 ms | delay before the temperature wheel / colour picker jumps to the current value on opening | `src/qml/components/entities/climate/deviceclass/Climate.qml:96`, `src/qml/components/entities/light/Color.qml:127` |

## 3. Input: long press, repeat, hold

| Value | What it controls | Where |
|---|---|---|
| 800 ms | how long a remote key must be held for its long-press action | `src/qml/components/ButtonNavigation.qml:553` |
| 3000 ms | how long the power button must be held for the power-off screen | `src/ui/inputController.cpp:31` |
| 150 ms | how long a key release is held back while the key auto-repeats, so a held key is not seen as released | `src/ui/inputController.cpp:269` |
| 300 ms | how long a cover up/down button must be held before it starts repeating | `src/qml/components/entities/cover/deviceclass/Window.qml:79` (+ `Garage`, `Curtain`, `Blind` `:79`) |
| 150 ms, then 40 ms after 5 steps | repeat rate while a cover up/down button is held | `src/qml/components/entities/cover/deviceclass/Window.qml:92,98,111` (+ 3 more) |
| 500 ms | press-and-hold on the battery icon toggles the percentage display | `src/qml/components/StatusBar.qml:306` |
| 200 ms | press-and-hold to pick up a page tile / group entry for reordering | `src/qml/components/PageSelector.qml:363`, `src/qml/components/group/GroupEdit.qml:372` |
| 100 ms | press-and-hold to pick up a tile in the edit mode of a page | `src/qml/components/Page.qml:732` |
| 200 ms | delay before a touch on a list counts as a press rather than a flick (11 occurrences) | `src/qml/components/Page.qml:29`, `src/qml/components/PopupMenu.qml:159`, `src/qml/components/entities/EntityList.qml:652` |
| 100 ms | same, on the activity bar and popup/selector lists (5 occurrences) | `src/qml/components/Page.qml:316`, `src/qml/components/IconSelector.qml:268`, `src/qml/components/SelectWidget.qml:308` |
| 200 ms | how long the selection highlight takes to slide to the next list row (29 occurrences) | `src/qml/components/Page.qml:27`, `src/qml/components/PopupMenu.qml:158`, `src/qml/settings/Settings.qml:55` |
| 150 ms | the same highlight, on the readiness-check list | `src/qml/components/ReadinessCheck.qml:538` |
| 2000 ms | how long a wrong PIN is shown in red before the keypad resets | `src/qml/keypad/KeyPad.qml:27` |
| 1000 ms | how long the last character of a password stays visible before it is masked (4 occurrences) | `src/qml/settings/settings/WifiPassword.qml:140`, `src/qml/settings/settings/WifiSetup.qml:434`, `src/qml/components/docks/Configure.qml:293`, `src/qml/components/integrations/fields/Password.qml:24` |

## 4. Power, standby and wake-up

| Value | What it controls | Where |
|---|---|---|
| 10–60 s (from the core) | display-off timeout, set with a slider in Settings > Power | `src/qml/settings/settings/Power.qml:253-254`, `src/config/config.cpp:572` |
| 10–300 s (from the core) | sleep timeout, set with a slider in Settings > Power | `src/qml/settings/settings/Power.qml:310-311`, `src/config/config.cpp:552` |
| 0–10 s, default 2 s (local setting) | "retry commands within N seconds after wake-up" | `src/qml/settings/settings/Power.qml:134-135`, `src/config/config.cpp:479` |
| 1000 ms | how long "Power off" / "Reboot" must be held down to trigger | `src/qml/components/Poweroff.qml:111`, `:169` |
| 2 s (2 × 1000 ms ticks) | countdown shown before the remote turns off with an open remote-control screen | `src/qml/components/RemoteOpen.qml:17`, `:62` |
| 3000 ms | power-button hold that opens the power-off screen | `src/ui/inputController.cpp:31` |

## 5. Integration and dock setup

| Value | What it controls | Where |
|---|---|---|
| max(1000 ms, lease/3) | how often the remote renews an integration setup session so it does not expire | `src/integration/integrationController.cpp:881`, `:889` |
| 1000 ms | countdown tick on the "setup limit expires in" banner | `src/qml/components/integrations/Configure.qml:286` |
| 30 s (sent to the core) | integration discovery window | `src/core/core.h:127` |
| 5 s (sent to the core) | fetching metadata from a discovered integration driver | `src/core/core.h:130` |
| 30 s (sent to the core) | dock discovery window | `src/core/core.h:209`, `src/core/core.cpp:1197` |
| 30 s (sent to the core) | command executed on a discovered dock during setup | `src/core/core.h:206` |
| 10000 ms, fires immediately | WiFi scan status poll while configuring a dock | `src/qml/components/docks/Configure.qml:163-164` |
| 500 ms | delay before the entity list is reloaded after "Manage entities" is closed (2 occurrences) | `src/qml/components/integrations/Info.qml:263`, `:348` |
| 300 ms | delay before the "Manage entities" tab bar resets to the first tab | `src/qml/components/integrations/ManageEntities.qml:37` |

## 6. WiFi

| Value | What it controls | Where |
|---|---|---|
| 2000 ms | how often the scan status is polled while a scan runs (2 occurrences) | `src/qml/onboarding/Wifi.qml:312`, `src/qml/settings/settings/Wifi.qml:359` |
| 10000 ms | pause between two network scans (2 occurrences) | `src/qml/onboarding/Wifi.qml:329`, `src/qml/settings/settings/Wifi.qml:376` |
| 30000 ms | how long joining a network may take during onboarding before "connection failed" | `src/qml/onboarding/Wifi.qml:343` |
| 10000 ms | how long the join spinner stays on a network row in the settings list | `src/qml/settings/settings/WifiNetworkList.qml:531` |
| 500 ms | delay before the network list is re-read after a network was added or changed (6 occurrences) | `src/hardware/wifi.cpp:429`, `src/qml/settings/settings/Wifi.qml:341`, `src/qml/settings/settings/WifiInfo.qml:271`, `:288`, `src/qml/settings/settings/WifiNetworkList.qml:240`, `:249` |
| 1500 ms | delay before the network list is re-read after a saved network was deleted | `src/hardware/wifi.cpp:358` |
| 500 ms / 1000 ms | on opening a WiFi page: read the known networks, then start a scan (2 pages) | `src/qml/onboarding/Wifi.qml:109-110`, `src/qml/settings/settings/Wifi.qml:387-388` |
| 0 or 10–60 s (from the core) | background scan interval for nearby networks, set with a slider | `src/qml/settings/settings/Wifi.qml:211-212`, `src/config/config.cpp:710` |
| 1000 ms | retry to reopen the touch-slider input device after a suspend/resume (Remote 3) | `src/hardware/ucr3/touchSliderUCR3.cpp:25` |

## 7. Media player and artwork

| Value | What it controls | Where |
|---|---|---|
| 15000 ms | how long downloading cover art may take before it is abandoned | `src/ui/entity/mediaPlayer.cpp:18`, `:627` |
| 1000 ms, 3 attempts | retry after a failed cover-art download (C++ colour extraction path) | `src/ui/entity/mediaPlayer.cpp:1091`, `:1096` |
| 1000 ms, 2 retries | retry after an image failed to load (QML image loader) | `src/qml/components/entities/media_player/ImageLoader.qml:24-25`, `:118` |
| 500 ms | delay before the spinner appears over an image that is still loading | `src/qml/components/entities/media_player/ImageLoader.qml:111` |
| 500 ms | delay before the loading screen appears while browsing media | `src/qml/components/entities/media_player/MediaBrowser.qml:272` |
| 800 ms | typing pause before a media search is actually sent | `src/qml/components/entities/media_player/MediaBrowser.qml:278` |
| 2000 ms | how long the "playing…" feedback stays on a browsed item | `src/qml/components/entities/media_player/MediaBrowser.qml:286` |
| 1000 ms | delay before the artwork shrinks when playback stops | `src/qml/components/entities/activity/MediaComponent.qml:99` |
| 2000 ms / (width × 25 ms) / 500 ms | scrolling title: pause, scroll out, pause, scroll back (6 copies of the same code) | `src/qml/components/entities/media_player/MarqueeText.qml:20,47-50`, `.../deviceclass/Tv.qml:410,435-438`, `Speaker`, `Receiver`, `Set_top_box`, `Streaming_box`, `activity/MediaComponent.qml:218,243-246` |
| 200 ms | how often a held touch-slider sends its new value (4 sliders) | `src/qml/components/TouchSliderVolume.qml:261`, `TouchSliderBrightness.qml:261`, `TouchSliderPosition.qml:261`, `TouchSliderSeek.qml:277` |
| 1000 ms | how long a touch-slider overlay stays visible after the finger is lifted (4 sliders) | `src/qml/components/TouchSliderVolume.qml:273`, `TouchSliderBrightness.qml:273`, `TouchSliderPosition.qml:273`, `TouchSliderSeek.qml:289` |
| 1000 ms | delay before the "unavailable" icon appears over an entity detail page | `src/qml/components/entities/BaseDetail.qml:178` |
| 1000 ms | how long the touch-slider test popup in settings stays open | `src/qml/settings/settings/TouchSlider.qml:336` |

## 8. Voice assistant

| Value | What it controls | Where |
|---|---|---|
| 15 s (sent to the core) | how long the assistant listens before the core stops the session | `src/ui/entity/voiceAssistant.cpp:63` |
| 15000 ms | how long the overlay waits for a reply before showing "It's taking longer than expected" | `src/qml/components/VoiceOverlay.qml:288` |
| 2000 ms | how long the result stays on screen before the overlay closes | `src/qml/components/VoiceOverlay.qml:277` |
| 500 ms | delay before an error message replaces the listening screen | `src/qml/components/VoiceOverlay.qml:265` |
| 200 ms | how often the equalizer bars redraw while listening | `src/qml/components/VoiceOverlay.qml:336` |
| 15000 ms | how long downloading the spoken answer may take | `src/voice.cpp:10`, `:89` |
| 3000 ms | how long the UI waits for the previous playback process to die before giving up on it | `src/voice.cpp:66` |
| 5000 ms | how long the UI waits for the audio player to start | `src/voice.cpp:79` |

## 9. Notifications and toasts

| Value | What it controls | Where |
|---|---|---|
| 4000 ms | how long a plain notification stays on screen | `src/qml/components/Notification.qml:75` |
| 1000 / 1000 / 2000 ms | pulsing bar on an actionable notification (attention loop) | `src/qml/components/ActionableNotification.qml:153-155` |
| 1000 / 1000 / 2000 ms | the same pulsing bar on the readiness-check popup | `src/qml/components/ReadinessCheck.qml:392-394` |
| — | actionable notifications have **no** auto-dismiss; they stay until the user acts | `src/qml/components/ActionableNotification.qml` |
| 2000 ms | how long an input-field error outline and message stay visible | `src/qml/components/InputField.qml:161`, `src/qml/components/SearchField.qml:188` |

## 10. Onboarding and navigation

| Value | What it controls | Where |
|---|---|---|
| 500 ms | delay before the previous onboarding step is shown, so the screen can slide out | `src/ui/onboardingController.cpp:97` |
| 5000 ms | how long each greeting is shown on the welcome screen | `src/qml/onboarding/Start.qml:34` |
| 3000 ms | delay before onboarding continues after the profile was created | `src/qml/onboarding/Profile.qml:64` |
| 1500 ms | how long the home button is ignored after a popup menu opens | `src/qml/components/PopupMenu.qml:59` |
| 400 ms | delay before a closing drop-down menu hands the keys back to the page | `src/qml/components/entities/DropDownMenu.qml:29` |
| 400 ms | delay before the activity menu resets to its first page after closing | `src/qml/components/entities/activity/deviceclass/Activity.qml:58` |
| 500 ms | delay before the activity "fix states" menu opens, once the screen has slid in | `src/qml/components/entities/activity/deviceclass/Activity.qml:358` |
| 1000 ms | delay before an activity screen is loaded into the second container | `src/qml/main.qml:69` |
| 200 ms | delay before the on-screen keyboard input gets the focus | `src/qml/main.qml:738` |
| 100 ms | delay before a page that left the view resets to its first item | `src/qml/components/Page.qml:147` |
| 100 ms | delay before the group list resets to its first item | `src/qml/components/group/Base.qml:133` |
| 500 ms | delay before the group list highlight is re-applied | `src/qml/components/group/Base.qml:143` |
| 1000 ms | delay before dragging is re-enabled after a group edit swipe | `src/qml/components/group/GroupEdit.qml:477` |

## 11. Loading indicators and spinners

| Value | What it controls | Where |
|---|---|---|
| 200 ms | delay before the status-bar spinner appears for a running entity command | `src/ui/entity/entityController.cpp:777` |
| 500 ms | delay before the spinner appears on a loading image / media browser | `src/qml/components/entities/media_player/ImageLoader.qml:111`, `.../MediaBrowser.qml:272` |
| 3000 ms | how long the full-screen loading overlay waits before offering "Cancel" | `src/qml/components/LoadingScreen.qml:351` |
| 180000 ms (3 min) | how long the full-screen loading overlay stays before it gives up | `src/qml/components/LoadingScreen.qml:396` |
| 5000 ms | how long the entity-list loading indicator stays before it gives up | `src/qml/components/entities/EntityList.qml:887` |
| 10000 ms | how long the WiFi join spinner stays before it gives up | `src/qml/settings/settings/WifiNetworkList.qml:531` |
| 3000 ms | how often the software update page re-checks for an update | `src/qml/settings/SoftwareUpdate.qml:32` |
| 1200 ms per turn | rotation speed of the small spinner in the status bar and on entity titles (3 occurrences) | `src/qml/components/StatusBar.qml:163`, `src/qml/components/entities/BaseTitle.qml:143`, `src/qml/components/entities/Base.qml:488` |
| 2000 ms per turn | rotation speed of the same spinner image during WiFi / dock / integration discovery (4 occurrences) | `src/qml/settings/settings/WifiNetworkList.qml:334`, `:524`, `src/qml/components/docks/Discovery.qml:295`, `src/qml/components/integrations/Discovery.qml:145` |
| 800 ms per turn | rotation of the large loading circle on the full-screen loading and voice overlays | `src/qml/components/LoadingScreen.qml:251`, `src/qml/components/VoiceOverlay.qml:555` |
| 1000 ms | how long the success/failure result stays before the loading overlay closes | `src/qml/components/LoadingScreen.qml:284`, `:325` |

## 12. Animations and transitions (grouped by duration, not per occurrence)

| Value | What it is used for | Occurrences | Examples |
|---|---|---|---|
| 300 ms | the house default: popup open/close fades, slide-ups, scale-ins, colour and opacity `Behavior`s, page overlays | ~320 | `src/qml/main.qml:503`, `src/qml/components/PopupMenu.qml:24`, `src/qml/components/Button.qml:83` |
| 200 ms | short feedback: small fades, list highlight moves, keypad and slider handle feedback, notification enter/exit | ~90 | `src/qml/components/Notification.qml:34`, `src/qml/components/Slider.qml:70`, `src/qml/components/ReadinessCheck.qml:127` |
| 100 ms | the shortest feedback: checkmark strokes, slider colour changes, small icon changes | ~46 | `src/qml/components/Slider.qml:97`, `src/qml/components/LoadingScreen.qml:280` |
| 0 ms | deliberately instant state changes (no animation) | 36 | `src/qml/components/SelectWidget.qml:199`, `src/qml/components/integrations/Info.qml:267` |
| 1 ms | resetting a rotation before a spinner loop restarts | 9 | `src/qml/components/LoadingScreen.qml:250` |
| 400 ms | popup opacity on the dock / integration / onboarding pages (paired with a 300 ms scale), greeting cross-fade, discovery indicator slides | ~28 | `src/qml/settings/Docks.qml:181`, `src/qml/onboarding/Start.qml:50`, `src/qml/components/ConnectionStatus.qml:51` |
| 500 ms | bigger movements: page and header slides, success/fail circle fills, edge auto-scroll while dragging, marquee pauses | ~38 | `src/qml/components/Page.qml:65`, `src/qml/components/LoadingScreen.qml:271`, `src/qml/components/Page.qml:791` |
| 600 ms | success/failure dot growth on the activity loading screen, image loader pulse, "manage entities" expand | 9 | `src/qml/components/entities/activity/LoadingScreen.qml:330`, `src/qml/components/entities/media_player/ImageLoader.qml:149` |
| 700 ms | the four rotating arcs of the first-start splash | 6 | `src/qml/components/LoadingFirst.qml:104` |
| 800 ms | spinner rotation on loading/voice overlays, image fade-in | 3 | `src/qml/components/LoadingScreen.qml:251`, `src/qml/components/entities/media_player/ImageLoader.qml:70` |
| 1000 ms | pulsing attention bar on actionable notifications and the readiness check | 4 | `src/qml/components/ActionableNotification.qml:153` |
| 1200 ms | small spinner rotation (status bar, entity titles), final splash pause | 4 | `src/qml/components/StatusBar.qml:163`, `src/qml/components/LoadingFirst.qml:131` |
| 2000 ms | large spinner rotation (discovery pages) and the pause before a title starts scrolling | 14 | `src/qml/components/docks/Discovery.qml:295`, `src/qml/components/entities/media_player/MarqueeText.qml:47` |
| 150 ms | second stroke of the success tick / failure cross | 9 | `src/qml/components/LoadingScreen.qml:281` |
| 75 ms | pause between the two strokes of the failure cross | 3 | `src/qml/components/LoadingScreen.qml:319` |
| 220 / 250 / 260 ms | one-off easing values on scroll indicators and the keyboard style | 7 | `src/qml/components/ScrollIndicator.qml:60`, `src/qml/keyboard/.../style.qml:445` |
| width × 25 ms | scroll time of a title that does not fit (≈40 px/s) | 8 | `src/qml/components/entities/media_player/MarqueeText.qml:20` |
| velocity 150 px/s | switch knob travel | 1 | `src/qml/components/Switch.qml:79` |

## 13. Everything else

| Value | What it controls | Where |
|---|---|---|
| 1000 ms | how long the keyboard's language indicator stays highlighted after a layout change | `src/qml/keyboard/QtQuick/VirtualKeyboard/Styles/remotestyle/style.qml:24` |
| 800 ms | how long a key's popup preview lives after the key is released | `src/qml/keyboard/QtQuick/VirtualKeyboard/Styles/remotestyle/style.qml:904` |
| 500 ms | how fast a list auto-scrolls while a tile is dragged past its edge (4 occurrences) | `src/qml/components/Page.qml:791`, `:800`, `src/qml/components/group/GroupEdit.qml:439`, `:448` |
| 1000 px/s² | flick deceleration on every scrollable list (~30 occurrences, a feel constant rather than a timing) | `src/qml/components/Page.qml:26` |

## Inconsistencies

1. **Two speeds for the same spinner image.** `loader_small.png` turns in **1200 ms** in the status bar, on entity titles and on entity detail pages (`src/qml/components/StatusBar.qml:163`, `src/qml/components/entities/BaseTitle.qml:143`, `src/qml/components/entities/Base.qml:488`) but in **2000 ms** on the WiFi network list, dock discovery and integration discovery (`src/qml/settings/settings/WifiNetworkList.qml:334`, `:524`, `src/qml/components/docks/Discovery.qml:295`, `src/qml/components/integrations/Discovery.qml:145`). Two identical-looking spinners rotate at different speeds side by side on the same screen when a command runs during a scan.

2. **Four different long-press thresholds.** Remote keys: **800 ms** (`src/qml/components/ButtonNavigation.qml:553`). Power button: **3000 ms** (`src/ui/inputController.cpp:31`). Battery icon: **500 ms** (`src/qml/components/StatusBar.qml:306`). Tile / group pick-up: **200 ms** (`src/qml/components/PageSelector.qml:363`, `src/qml/components/group/GroupEdit.qml:372`) but **100 ms** for the same gesture on a page in edit mode (`src/qml/components/Page.qml:732`). The 200/100 pair is the same user action with two values.

3. **Two press delays for the same kind of list.** **200 ms** on most lists (`src/qml/components/Page.qml:29`, `PopupMenu.qml:159`, `PopupList.qml:211`, `entities/EntityList.qml:652`, `ProfileSwitch.qml:295`, `SettingsNew.qml:299`, `settings/Docks.qml:79`, `settings/Integrations.qml:82`, `settings/About.qml:145`, `components/Profile.qml:687`, `integrations/Info.qml:22`) and **100 ms** on five others (`src/qml/components/Page.qml:316`, `IconSelector.qml:268`, `:322`, `SelectWidget.qml:308`, `entities/select/deviceclass/Select.qml:96`, `entities/media_player/SourceList.qml:110`). `Page.qml` uses both: 200 ms for the page itself and 100 ms for its tile list.

4. **Joining a WiFi network gives up after 30 s in onboarding but 10 s in settings.** Onboarding shows "connection failed" after **30000 ms** (`src/qml/onboarding/Wifi.qml:343`, with a comment saying the previous 3 s budget was always too short), while the settings list only hides the join spinner after **10000 ms** and has no failure message at all (`src/qml/settings/settings/WifiNetworkList.qml:531`). The two WiFi pages are otherwise copies of each other (identical 2000/10000 scan timers).

5. **Refreshing the WiFi network list uses 500 ms in six places and 1500 ms in one.** Adding or changing a network re-reads the list after **500 ms** (`src/hardware/wifi.cpp:429` plus five QML call sites), deleting a saved network after **1500 ms** (`src/hardware/wifi.cpp:358`).

6. **Two retry policies for the same artwork.** The C++ download retries **3 times, 1000 ms apart** (`src/ui/entity/mediaPlayer.cpp:1091`, `:1096`); the QML image loader retries **2 times, 1000 ms apart** (`src/qml/components/entities/media_player/ImageLoader.qml:24-25`). The same failing cover-art URL is therefore attempted 3 times for the colour extraction and 3 times for the display, with no shared budget.

7. **Three different "give up on the spinner" timeouts.** Full-screen loading overlay **180000 ms** (`src/qml/components/LoadingScreen.qml:396`), entity list **5000 ms** (`src/qml/components/entities/EntityList.qml:887`), WiFi join **10000 ms** (`src/qml/settings/settings/WifiNetworkList.qml:531`). None of them relates to the 10 s core request timeout (`src/core/core.cpp:16`), so a spinner can outlive or predecease the request it represents.

8. **Popup exit fades are 300 ms except for three popups that use 200 ms.** 300 ms in ~20 popups (`src/qml/components/PopupMenu.qml:32`, `TouchSlider.qml:232`, `VolumeOverlay.qml:94`, `Poweroff.qml:27`, `RemoteOpen.qml:24`, `settings/settings/WifiJoin.qml:36`, …) versus **200 ms** in `src/qml/components/Notification.qml:41`, `src/qml/components/ActionableNotification.qml:124` and `src/qml/components/ReadinessCheck.qml:127`. `ActionableNotification` also enters in 300 ms and leaves in 200 ms, and `Notification` enters with four animations of mixed 200/300 ms.

9. **The dock and integration popups fade in 400 ms while scaling in 300 ms**, so the two halves of one animation end 100 ms apart (`src/qml/settings/Docks.qml:180-181`, `:187-188`, `src/qml/settings/Integrations.qml:186-187`, `:193-194`, `src/qml/onboarding/Dock.qml:132-133`, `src/qml/onboarding/Integration.qml:124-125`). The same popup type in `src/qml/settings/Integrations.qml:148-149` uses 300/300.

10. **List highlight move: 200 ms everywhere, 150 ms in one place** (`src/qml/components/ReadinessCheck.qml:538` against 29 uses of 200 ms).

11. **Retry-after-failure intervals cluster at 2000 ms but the core reconnect shares that value with unrelated retries.** Core reconnect **2000 ms** (`src/core/core.h:548`), configuration reload **2000 ms** (`src/config/config.cpp:969`), profile reload **2000 ms** (`src/ui/uiController.cpp:241`), command resend after a wake-up **500 ms** (`src/ui/entity/entityController.cpp:779`), readiness re-check **1000 ms** (`src/qml/main.qml:228`). Four different "try again" cadences, none derived from one another, and none backs off.

12. **Two "delay before the loading indicator appears" values.** **200 ms** for entity commands (`src/ui/entity/entityController.cpp:777`) and **500 ms** for images and media browsing (`src/qml/components/entities/media_player/ImageLoader.qml:111`, `.../MediaBrowser.qml:272`).

13. **Auto-dismiss of transient overlays ranges from 1000 to 4000 ms with no rule.** Touch slider **1000 ms** (`src/qml/components/TouchSliderVolume.qml:273` + 3 siblings), volume overlay **2000 ms** (`src/qml/components/VolumeOverlay.qml:191`), voice overlay **2000 ms** (`src/qml/components/VoiceOverlay.qml:277`), media-browser play feedback **2000 ms** (`.../MediaBrowser.qml:286`), notification **4000 ms** (`src/qml/components/Notification.qml:75`), input error **2000 ms** (`src/qml/components/InputField.qml:161`).

14. **Two polling cadences for the same WiFi scan state.** The WiFi pages poll the scan status every **2000 ms** while a scan runs (`src/qml/onboarding/Wifi.qml:312`, `src/qml/settings/settings/Wifi.qml:359`), the dock setup page polls the same state every **10000 ms** (`src/qml/components/docks/Configure.qml:163`).

15. **Deferred hand-back of the key input uses 400 ms in two places and 300/500 ms in others.** Drop-down menu **400 ms** (`src/qml/components/entities/DropDownMenu.qml:29`), activity menu **400 ms** (`.../Activity.qml:58`), manage-entities tab reset **300 ms** (`src/qml/components/integrations/ManageEntities.qml:37`), integration entity reload **500 ms** (`src/qml/components/integrations/Info.qml:263`, `:348`) — all of them "wait for the 300 ms close animation".

16. **A live 4 s ceiling sits inside a 10 s one.** The readiness check gives up after **4000 ms** (`src/ui/entity/entityController.cpp:24`) while the underlying core request runs for **10000 ms** (`src/core/core.cpp:16`); the response can therefore arrive after the UI has already declared the check failed. This one is deliberate and documented in the code, but it is the only place where the two ceilings are related at all.

17. **Dead keep-alive.** `m_keepAliveInterval = 60000` and `startKeepAliveTimer()` / `stopKeepAliveTimer()` still exist but the timer is never constructed (`src/core/core.h:544-545`, `src/core/core.cpp:26-28`, `:1385-1396`), so calling either would dereference an uninitialised pointer. There is no websocket keep-alive at all today.

## Configurable values

| Value | Source | Range / default | Where |
|---|---|---|---|
| Core request timeout | environment variable `UC_UI_REQUEST_TIMEOUT` | default 10000 ms when unset or 0 | `src/core/core.cpp:14-17` |
| Display-off timeout | core (`power_saving` config), slider in Settings > Power | 10–60 s | `src/config/config.cpp:572`, `:1086`, `src/qml/settings/settings/Power.qml:253-254` |
| Sleep (standby) timeout | core (`power_saving` config), slider in Settings > Power | 10–300 s | `src/config/config.cpp:552`, `:1083`, `src/qml/settings/settings/Power.qml:310-311` |
| Wake-up retry window | local `config.ini` key `ui/resumeTimeoutWindow`, slider in Settings > Power | 0 (off) – 10 s, default 2 s | `src/config/config.cpp:478-484`, `src/qml/settings/settings/Power.qml:134-135` |
| WiFi background scan interval | core (`network` config), slider in Settings > WiFi | 0 (off) or 10–60 s, step 5 | `src/config/config.cpp:710`, `:1076`, `src/qml/settings/settings/Wifi.qml:211-213` |
| Integration setup keep-alive lease | core, per setup session (`keepalive_timeout_sec`) | renewal interval = max(1000 ms, lease/3) | `src/core/structs.h:158`, `src/integration/integrationController.cpp:881` |
| Integration setup expiry countdown | core (`setup_expires_in_sec`) | displayed, counted down locally every 1000 ms | `src/qml/components/integrations/Configure.qml:286` |
| Activity timeout | core, per activity entity attribute | reported only, the UI does not time anything out itself | `src/ui/entity/activity.cpp:210-212` |
| Integration / dock discovery windows | defaults in the UI, sent to the core | 30 s discovery, 5 s driver metadata, 30 s dock command | `src/core/core.h:127`, `:130`, `:206`, `:209` |
| Voice assistant listening timeout | default in the UI, sent to the core | 15 s when the caller passes 0 (every call site does) | `src/ui/entity/voiceAssistant.cpp:63` |
