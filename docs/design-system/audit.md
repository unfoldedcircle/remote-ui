# Remote UI Readability Audit

Every screen of the Remote Two / Remote 3 on-device UI (remote-ui, Qt 5 / QML) reviewed for readability on the Remote 3 LCD and for consistent d-pad navigation, with a proposed design system to fix both.

Repository: unfoldedcircle/remote-ui at release v0.82.1; all file:line references point there. Date: 2026-09-16,
revised 2026-10-07. Status: decisions accepted, no code changed yet. The rules that came out of it are in the
[design system](../design-system.md); the screens are in the [mockups](mockups/README.md).

> **Revision of 2026-10-07.** The seven decisions are accepted. The designer revised the mockups: the main UI marks the selection with a lighter fill, settings mark it with a 3px ring, and the ring sits on the control rather than the whole section. Sections 7 to 10 follow that revision. The selected fill is provisionally #595959 for the hardware check. Four questions are still open (section 10).

## 1. Summary

The UI has one palette, generated in `src/ui/colors.cpp`, tuned for a panel with a true black. On the Remote 3 the black is a dark grey and the two things you notice daily fall straight out of the numbers:

- **The d-pad selection is a fill of `colors.dark` (#161616) on black.** That is 1.16:1 on the OLED and about 1.1:1 on the LCD. It is used by the Settings menu, the settings overlay, the About menu, the popup menu, every popup list (language, country, timezone, dropdowns), the localisation and Wi-Fi band selectors, the entity list, the profile menus and the home-page tiles. The 1px border around it is `Qt.lighter(dark, 1.3)`, #1D1D1D, which adds nothing.

- **Secondary text is `colors.light` (#787878) in 24px Space Mono.** 4.8:1 on the OLED, about 3.6:1 on an LCD in a lit room, in the least legible face at a small size. It carries every help text, the release notes, the terms, the licences, driver instructions, list subtitles and the web-configurator URL.

Beyond those two, the audit found **seven different ways to draw the selected element**, of which only one (a 2px `colors.highlight` outline) reads on the LCD; **six side-gutter widths** (0, 10, 20, 30, 40 px); **five key/value row styles**; four title-bar styles; secondary text in seven colour/size combinations; and a dozen tappable controls that a d-pad user cannot reach at all.

> **The fix is mostly a token and one-component change.** Retuning the palette values in one C++ file lifts all secondary text and surfaces at once. A single shared selection component with two styles, a light fill on the main UI and a 3px ring in settings, replaces the seven patterns. The rest is a layout normalisation done screen by screen behind an agreed rule set. Nothing here needs a new visual identity: Poppins, Space Mono, black background and the offwhite text stay.

## 2. Scope, method, assumptions

**Scope.** All 224 QML files under `src/qml/`: settings (level 0 to 3), onboarding, shared components, docks and integrations flows, entity tiles and every device-class detail screen, keypad and the on-screen keyboard style. The C++ colour and font providers. Not in scope: the web configurator, the core simulator, the button simulator.

**Method.** Each file was read for (a) every text colour/face/size combination and its role, (b) how the d-pad selection is rendered and gated, (c) row heights, gutters, dividers, radii, icon sizes, (d) reachability by d-pad. Contrast ratios are WCAG 2.x relative luminance, computed from the values `Colors::generateColorPalette` produces for the default base colour (black, therefore achromatic). Counts come from grepping the sources.

**LCD model.** No panel data sheet is in either repository. The "LCD" column assumes a black level of 2% of peak white (a 50:1 effective contrast once ambient reflection is included, typical for a small IPS panel in a lit room) and rescales every luminance accordingly. Treat it as an estimate that ranks risks correctly, not as a measurement. The physical diagonal is also not documented; text sizes below are in pixels of the 480 x 850 canvas. The Remote 3 panel is 480 x 800 and the Remote Two 480 x 854, so vertical fit is checked at 800px.

**Not verified on hardware.** Nothing was run on a Remote 3 for this audit. The two reported issues were reproduced from the sources; the rest are predictions from the same numbers.

## 3. Current state, measured

### 3.1 Palette

What `colors.cpp` produces with the default base colour, and how each pair reads. AA for body text is 4.5:1, for large text 3:1; 3:1 is also the floor for a non-text indicator such as a selection outline.

| Token | Value | Role in the UI | On black, OLED | On black, LCD est. | Verdict |
|---|---|---|---|---|---|
| offwhite | #D0D0D0 | Primary text, icons | 13.6:1 | 9.8:1 | fine |
| light | #787878 | Secondary text, help, prose, inactive icons | 4.8:1 | 3.6:1 | fails AA on LCD |
| inactiveText | #606060 | Step numbers, "not needed" states | 3.3:1 | 2.6:1 | fails |
| highlight | #C8C8C8 | 2px focus outline on buttons, switches, some rows | 12.6:1 | 9.1:1 | fine, under-used |
| dark | #161616 | Selected row fill, sheets, cards, fields, slider tracks | 1.16:1 | 1.11:1 | invisible as a selection |
| medium | #232323 | 2px dividers, switch track, 1px card and selection borders | 1.34:1 | 1.24:1 | hairlines vanish |
| primaryButton | #5A5A5A | Button fill (offwhite label) | 4.5:1 label | 4.0:1 label | borderline |
| secondaryButton | #1E1E1E | Cancel button fill | 1.26:1 | 1.18:1 | button body invisible |
| red | #FF3E54 | Errors, destructive | 6.1:1 | 5.0:1 | fine |
| green | #769990 | Success | 6.7:1 | 5.4:1 | fine |
| blue | #335266 | Rare | 2.5:1 | 2.2:1 | never for text |

A base colour with a hue (the disabled Colors settings page) makes it worse, not better: for hue 210 the "light" token drops to 4.1:1 and "dark" to 1.08:1.

### 3.2 Typography inventory

Two faces, both called with pixel sizes at each site. There is no scale: 50 distinct size/weight calls exist across the sources. The most used:

| Call | Sites | Typical role |
|---|---|---|
| primaryFont(30) | 100 | Row labels, setting labels, tile names |
| secondaryFont(24) | 89 | Help text, prose, states, key/value keys (almost always in `light`) |
| secondaryFont(20) | 39 | Timestamps, MAC, versions, times, keyboard keys |
| secondaryFont(22) | 35 | Subtitles, hints, tab labels, badges |
| primaryFont(26) | 29 | Dialog titles, list row names |
| primaryFont(24) | 25 | Title bars, onboarding titles, detail titles |
| primaryFont(20) | 11 | Toast text, software update labels, hints |

Below 24px there are 95 call sites, the smallest being 16px (repeat badge), 18px (input error, network sub-line) and a computed 19.2px (activity media widget artist). Poppins Thin at 120px (integration entity count) and Light at 36px (PIN digits) rely on hairline strokes.

### 3.3 Selection rendering inventory

Seven distinct renderings of "this is where the d-pad is". Only pattern A is visible on the LCD.

| # | Rendering | Contrast vs. rest state | Where |
|---|---|---|---|
| A | 2px `highlight` outline, no fill | 12.6:1 | Button, Switch, Checkbox, RowHighlight, WifiNetworkList:421, Activity.qml:653, ReadinessCheck, Discovery cards, Dropdown, hand-rolled copies (SoftwareUpdate:252, docks/Info:246, Configure:419, integrations/Info:322) |
| B | `dark` fill + 1px `Qt.lighter(dark,1.3)` or `medium` border | 1.16:1 | Settings.qml:131, SettingsNew.qml:416, PopupMenu.qml:211, PopupList.qml:307, Localisation.qml:416, Wifi.qml:421, EntityList.qml:896, Profile.qml:804, ProfileSwitch.qml:494, About.qml:362 (no border), PageSelector.qml:565 (no border) |
| C | `Qt.darker(medium)` ≈ #111 fill + 1px `medium` border, always on | 1.07:1 | entities/Base.qml:28 (every home-page tile), group/Base.qml:15 |
| D | `medium` fill, no border | 1.34:1 | MediaBrowser.qml:959; SourceList.qml:130 (dark + 2px #2C2C2C) |
| E | 2px `medium` border, escalating to highlight when held | 1.34:1 | Page.qml:752 (reorder), GroupEdit.qml:397 |
| F | `offwhite` fill with `dark` text (inversion) | 11.7:1 | SelectWidget.qml:325, select/Select.qml:112 (doubles as "current value") |
| G | 3px `highlight` bar at the left edge; 2px `offwhite` border; 1px `light` border; text colour change only | varies | ReadinessCheck.qml:609; IconSelector.qml:287; InputField.qml:35; Poweroff.qml:229 (Cancel) |

Four sites render a selection permanently instead of only while the keypad is active (`ui.keyNavigationActive`): the home tiles, group tiles, SelectWidget and SourceList; `Activity.qml` keeps its own flag instead of the global one.

### 3.4 Layout variance

**Side gutters.** 0 (dialog fields and buttons: WifiPassword, WifiSetup, Rename, PasswordChange, Terms, RemoteName, LicensePage text), 10 (settings pages, tiles, lists), 20 (onboarding, cards, sheets), 30 (detail screens), 40 (integrations Settings, Finish, Skip in Integration.qml).

**Title bars.** 60px with Poppins 24 and a 60px arrow (TopNavigation); 80px arrow with Poppins 26 (SettingsNew, WebConfig); Poppins 30 with an X (ManageEntities); Poppins 26 without any icon (dialogs); no title and a touch-only X at -20px (Info popups).

**Row heights.** 60 (Wi-Fi band selector), 80 (menus, network rows), 100 (dock actions, activity menu), 110/120 (two-line rows, localisation selector, profile switch), 130 (tiles), 150 (page selector, group edit).

**Dividers.** 2px `medium` at width-20 on settings pages; 1px `medium` in About, AboutInfo, Info popups; 2px `medium` on a `dark` sheet (WifiInfo, 1.15:1); 1px `Qt.lighter(medium)` in EntityList filters; 10px `dark` blocks in Garage.

**Corner radii.** Tokens 8 and 22 almost everywhere, plus literal 5 (keyboard), 8 (Page.qml:651), 10 (MediaBrowser rows), 16 (coverflow), 34/35 (pills).

**Icons.** Boxes of 40/60/70/80/100/140 with the glyph at half the box. Back arrow 60 or 80, close X 60/70/80/100, integration icon 60 or 80, marker icons down to a 13px glyph (ReadinessCheck).

## 4. Readability register

Every finding that makes text or a control hard to see, grouped by cause. Severity: high daily-use screens or invisible on the LCD, medium secondary screens, low rare or cosmetic. File paths are under `src/qml/`.

| ID | Finding | Where | Severity | Rule that fixes it |
|---|---|---|---|---|
| `R-01` | D-pad selection drawn as `dark` fill on black (pattern B and C above) | settings/Settings.qml:131, components/SettingsNew.qml:416, PopupMenu.qml:211, PopupList.qml:307, entities/Base.qml:28, entities/EntityList.qml:896, settings/settings/Localisation.qml:416, Wifi.qml:421, About.qml:362, Profile.qml:804, ProfileSwitch.qml:494, PageSelector.qml:565 | high | Selection contract (7.4): light fill on the main UI, 3px ring in settings |
| `R-02` | Long prose in `light` Space Mono 24: release notes, legal, licences, driver instructions, terms, reset warning, driver labels | settings/softwareupdate/ReleaseNotes.qml:59, settings/about/AboutPage.qml:89, LicensePage.qml:232, integrations/UserAction.qml:434 and 462, integrations/fields/Label.qml:211, onboarding/Terms.qml:37, settings/settings/Reset.qml:384 | high | Prose is textPrimary Poppins 26, line-height 1.3; mono only for values |
| `R-03` | Help text under every setting in `light` Space Mono 24 | settings/settings/Ui.qml:78-373 (7 sites), Display.qml:384/481, Power.qml:93/125/187, Voice.qml:232/365, Wifi.qml:203, SoftwareUpdate.qml:405/460, TouchSlider.qml:94/109 (Poppins 24), docks/Finish.qml:485/545 | high | Help = textSecondary (#A0A0A0) Poppins 26 |
| `R-04` | Grey text at 22px or smaller: versions, "Current version" row, download state, percent, beta toggle label, timestamps, MAC, entity ids, hints, tab labels, badges, QR captions | settings/SoftwareUpdate.qml:96-110/164-179/217/344-357, softwareupdate/UpdateProgress.qml:369, NotificationDrawer.qml:136, onboarding/Wifi.qml:191-211, entities/EntityList.qml:938, docks/Info.qml:289, Voice.qml:307/324, docks/Configure.qml:228/326/379, docks/Discovery.qml:224/377, integrations/Configure.qml:308, integrations/Discovery.qml:257, WebConfig.qml:226/255, SettingsNew.qml:383-403, ManageEntities.qml:150/171, Activity.qml:487, IconSelector.qml:206/232 | high | Minimum 22px, and only for captions; 22px text is never in a colour below textSecondary |
| `R-05` | Smallest text: 16px repeat badge, 18px input error and network sub-line, 19.2px artist | media_player/deviceclass/Tv.qml:620, activity/MediaComponent.qml:422 and 266, components/InputField.qml:151, settings/settings/WifiNetworkList.qml:446 | medium | Floor 22px; errors 24px red |
| `R-06` | Text de-emphasised with opacity (0.3 to 0.75) instead of a colour: notification body, "Restricted", discovery headers, MAC, progress, tile counts, unreached steps, empty select value | ActionableNotification.qml:250, Profile.qml:663, SettingsNew.qml:275, WebConfig.qml:410, docks/Discovery.qml:273, integrations/Discovery.qml:123/175, AddEntities.qml:70, About.qml:342, group/Base.qml:231, ReadinessCheck.qml:559, SelectWidget.qml:126, onboarding/Wifi.qml:209, RemoteOpen.qml:51, activity/LoadingScreen.qml:633 | medium | Opacity only for disabled (0.4) and overlays; emphasis by token |
| `R-07` | `inactiveText` (#606060) for information: step numbers and "not needed" states | components/ReadinessCheck.qml:561, 205, 602 | medium | textDisabled only on disabled controls |
| `R-08` | Text in `medium` (#232323): track duration, effectively invisible on any panel | media_player/MediaBrowser.qml:1031 | high | Values are textPrimary mono |
| `R-09` | `light` text on `dark` surfaces (4.1:1 OLED): sheet keys, card addresses, integration switch label | settings/settings/WifiInfo.qml:182/211/240, docks/Configure.qml:228, integrations/Info.qml:369, docks/Discovery.qml:377 | medium | textSecondary on surface is 6.4:1 |
| `R-10` | Hairline 1px `medium` lines and card borders; 2px `medium` dividers on `dark` sheets | About.qml:350, AboutInfo.qml:29/80, docks/Info.qml:494/536, integrations/Info.qml:400, docks/Configure.qml:179, ManageEntities.qml:136, WifiInfo.qml:168, ReadinessCheck.qml:451, EntityList.qml:495 | medium | Dividers 2px in divider colour (#3A3A3A) |
| `R-11` | Control bodies in `dark`/`medium`: Cancel (secondaryButton) on black and on a dark sheet, Identify/Connect buttons, progress tracks, fields with 0-width borders, volume track, EQ bars | WifiJoin.qml:245, PageAdd/PageRename/ProfileAdd/ProfileRename/GroupRename, docks/Info.qml:320, docks/Discovery.qml:456, SoftwareUpdate.qml:194, UpdateProgress.qml:352, InputField.qml, Dropdown.qml:100, VolumeOverlay.qml:136, VoiceOverlay.qml:350 | medium | Secondary button = surface + 2px divider border; tracks in surfaceRaised |
| `R-12` | Entity controls that are a `dark` square inside a `medium` square (off state), slider fills in `Qt.darker(light)` on `dark`, 4px `medium` ring | button/deviceclass/Button.qml:61, Macro.qml:64, switch/deviceclass/Switch.qml:74, Outlet.qml, light/OnOff.qml, cover/deviceclass/Window.qml:229, Garage.qml:229, Blind.qml:241, Curtain.qml:252, BaseOnOffButton.qml:30, Climate.qml:387 | medium | Off state = surface with 2px divider outline; slider fill textSecondary |
| `R-13` | Thin and Light weights at small size: PIN digits Poppins 36 Light, entity count Poppins 120 Thin | WebConfig.qml:310, onboarding/Finish.qml:171, Profile.qml, integrations/Info.qml:309 | low | Light only at 56px and above; never Thin |
| `R-14` | Overlay dims of 0.5 to 0.85 black over a black page: on an LCD the sheet and the page merge because both are grey | ConnectionStatus.qml:68 (0.5), EntityList.qml:408 (0.6), BottomSheet.qml:72, SourceList.qml:71, MediaBrowser.qml:511, SelectWidget.qml:268 (0.8), BaseDetail.qml:161 (0.85); DropDownMenu.qml has no dim at all | medium | Bottom sheets are black with a 2px divider edge over a 0.85 dim |
| `R-15` | Low-opacity chrome: scroll bars at 0.5, pull-down menu circles at 0.1, search/field icons at 0.5, touch-slider pill border white at 0.3, page dots and PIN dots at 0.6 | PageSelector.qml:290, ProfileSwitch.qml:302, ReleaseNotes.qml, MainContainer.qml:516, SearchField.qml:151, InputField.qml:182, TouchSlider*.qml:163, Page.qml:699, KeyPad.qml:136 | low | Scroll indicator = surfaceRaised track, textSecondary thumb |
| `R-16` | Pressed state fills `offwhite` under offwhite content: the label vanishes while pressed | ButtonAdd.qml:41, ButtonIcon.qml:42, IconSelector.qml:302, keypad/Key.qml:30, Button.qml (text stays offwhite on offwhite) | low | Pressed inverts: textPrimary fill, black content |
| `R-17` | `lineHeight: 0.8` on Space Mono clips descenders | settings/settings/Reset.qml:460/470, ActionableNotification.qml:252, activity/Activity.qml:690, integrations/Configure.qml:367 (0.9) | low | Line height 1.2 to 1.3 |
| `R-18` | Web-configurator URL, the most useful string on the device, in `light` Space Mono 22 | components/WebConfig.qml:255, onboarding/Finish.qml:130, Profile.qml:446 | high | Value style: textPrimary Space Mono 26 |
| `R-19` | Keyboard keys 24px Space Mono, off-palette hard-coded colours (#80c342, #35322f, #1e1b18) | keyboard/QtQuick/VirtualKeyboard/Styles/remotestyle/style.qml:59-81, 266, 438, 541 | low | Keys 28px Poppins; palette tokens |

## 5. Inconsistency register

Where the same thing is done in more than one way. Each row names the variants found and the single rule proposed.

| ID | Topic | Variants observed | Proposed rule |
|---|---|---|---|
| `I-01` | Selection rendering | Seven patterns (section 3.3) | One `Components.Selectable` with two styles: `surfaceSelected` fill on the main UI, 3px `focusRing` in settings; bound to `ui.keyNavigationActive` |
| `I-02` | Selection gating | Bound to keyNavigationActive (most); always on (entities/Base, group/Base, SelectWidget, SourceList, MediaBrowser); own flag (Activity.qml:38) | Always `ui.keyNavigationActive`; a "current value" is shown with a check mark, not with the selection style |
| `I-03` | Side gutter | 0, 10, 20, 30, 40 px | 20px page gutter; selectable rows and tiles start 8px from the edge with content at 20 |
| `I-04` | Title bar | TopNavigation 60/P24/arrow 60; SettingsNew and WebConfig 80-arrow/P26; ManageEntities P30 + X; dialogs P26 no icon; Info popups no title, touch-only X; onboarding P24 no back | One 80px title bar: 80px back or close target, Poppins 28 Medium, used by pages, sheets and dialogs alike |
| `I-05` | Secondary text style | Space Mono 24 light; Poppins 24 light (TouchSlider); Poppins 22 light (Voice); Poppins 20 light (Voice, SoftwareUpdate); Space Mono 22 light (Discovery, Configure, WebConfig); Space Mono 24 offwhite at 0.6 (Discovery headers, AddEntities, Restricted); Space Mono 20 light (docks/Info) | Help/secondary = Poppins 26 textSecondary; caption = Poppins 22 textSecondary |
| `I-06` | Key/value rows | About.qml (P20 / M20 at 0.7, 1px line); AboutInfo (M24 light / P24, 1px); SoftwareUpdate (P20 light / M20 light, 2px); WifiInfo (stacked M24 light / M24, 2px on dark); selector rows (P30 / P20 Bold) | One `Components.KeyValueRow`: key Poppins 26 textSecondary, value Space Mono 26 textPrimary, 2px divider |
| `I-07` | Row heights | 60, 80, 100, 110, 120, 130, 150 | 80 single line, 110 two lines, 130 tiles; 150 only for reorder rows with a handle |
| `I-08` | Dividers | 2px medium (settings), 1px medium (About, Info), 1px lighter(medium) (EntityList), 2px medium on dark (sheets), coded as width-20 vs Layout margins | 2px divider colour inset 20, one `Components.Divider` |
| `I-09` | Buttons | 80px Space Mono 26 primaryButton; Cancel secondaryButton; 50px Space Mono 20 medium (Identify/Connect); bare text captions with a ring (LoadingScreen, delete drawers, notifications, cover Stop, Climate Mode/Fan, EntityList Clear/Done in light); ButtonAdd Poppins 30; pills 120x70 (MediaBrowser); Reset Confirm offwhite/black; OpenClose buttons in medium | Three button variants: primary, secondary (surface + 2px divider border), destructive; 80px, Poppins 26 Medium; text-only actions are not allowed |
| `I-10` | Skip / Next placement in onboarding | Wifi: primary width-20; Dock: secondaryButton width-20; Integration: primary width-40; Terms and RemoteName: full width, 0 margin | Full width inside the 20px gutter; Skip is secondary, Next is primary |
| `I-11` | Dialog forms | PageAdd/PageRename/ProfileAdd/ProfileRename/GroupRename/WifiPassword/WifiSetup/Rename/PasswordChange: full-width field with 0 margin, buttons flush to the screen edge; docks/Configure: 20px | One `Components.FormDialog`: title bar, field and buttons inside the 20px gutter |
| `I-12` | Mono vs sans for the same role | Field labels Space Mono 30 (FieldBase) vs Poppins 26 (docks/Configure); slider values Space Mono 24 light (Power) vs Poppins 30 offwhite (TouchSlider); drawer titles Space Mono 28; tab captions Space Mono 22; action captions Space Mono 30 Bold (cover, climate); BottomSheet title Space Mono 28 | Poppins for labels, prose, buttons, titles; Space Mono only for values (numbers, versions, addresses, codes, times, units) |
| `I-13` | Title sizes | Poppins 24 (TopNavigation, onboarding, BaseTitle), 26 (dialogs, EntityRename, Filters), 30 (Timezone confirm, EntityList, MediaBrowser, ManageEntities), 32 (ReadinessCheck detail), 50 (integrations Info name) | Title 28 Medium; section heading 26 Medium; only content values go bigger |
| `I-14` | Icon sizes | Back arrow 60/80; close X 60/70/80/100; integration icon 60/80; globe 30/40; marker glyphs 13px; plus glyph drawn 1px (NoPage) vs 2px (ButtonAdd, PageSelector, ProfileSwitch) | Icon boxes 60 (row), 80 (bar, close), 100 (tile); glyph strokes 2px |
| `I-15` | Corner radii | Tokens 8/22 plus literals 5, 8, 10, 16, 34, 35; `Qt.darker`/`Qt.lighter` derived fills at 9 sites | Only `ui.cornerRadiusSmall`/`Large`; no derived colours outside colors.cpp |
| `I-16` | Sheets and popups | Sheet fill `Qt.darker(dark,1.5)` (BottomSheet), `dark` (WifiInfo, DropDownMenu), `medium` (ConnectionStatus), black (SelectWidget, PopupMenu with gradient); dim 0.5/0.6/0.8/0.85/none | Bottom sheet = black, 22px top radius, 2px divider edge, 0.85 dim; popup card = surfaceRaised; popup menus end with a Close row |
| `I-17` | Scroll affordance | ScrollIndicator; ScrollBar at 0.5; both (AboutPage); none (Ui, Display, Power, Voice, TouchSlider, SoftwareUpdate, Localisation, Wifi, Info popups); scroll step height/2, 100px, 200px | One scroll indicator on every scrolling page; d-pad step = half the viewport |
| `I-18` | Capitalisation and wording | "Unit System", "Voice Assistant", "Known Networks", "Release Notes" vs "24-hour time", "Speech response", "Check for update"; "Uk"/"Us"; "Led brightness"; "Wifi & Bluetooth" vs "WiFi band" vs "Wifi network"; untranslated "Enabled"/"Disabled" (WifiNetworkList:447), untranslated notification (AdminPin:86) | Sentence case everywhere; "Wi-Fi"; all strings through qsTr |
| `I-19` | Disabled state | Opacity 0.3 (docks/Info rows, integrations switch, SoftwareUpdate, Configure), 0.5 (tiles, OpenClose), text colour only, mouseArea disabled without any visual (Localisation:451) | Disabled = opacity 0.4 on the whole control, no separate colour |
| `I-20` | Row alignment | PageSelector rows centred Poppins 50 Light; every other list left-aligned | Rows left-aligned, label 30 Regular |
| `I-21` | Duplicated screens drifting apart | Activity.qml header re-implements BaseTitle; Tv vs Receiver now-playing; four sensor files; four cover classes; five rename dialogs; TouchSlider* four copies; docks and integrations delete drawers | Shared components for header, key/value, form dialog, delete drawer; one sensor and one cover screen with a device-class parameter |
| `I-22` | Spacing outliers | integrations/Settings.qml:168 spacing 60; Wifi.qml:63 content starts 10px lower; TouchSlider no dividers between sliders; Sound.qml no help text; Voice.qml no divider before "Speech response"; slider sections childrenRect vs +40 | Section spacing 20, dividers between every section, help text on every setting or on none |
| `I-23` | Colour tokens leaking | `colors.white` in TouchSlider* and TouchSlider test popup; hard-coded hex in keyboard style and light/Brightness.qml:232; `Qt.lighter(mediaImageColor, 3)` progress fills with unpredictable contrast | Only palette tokens in QML; media-tinted fills clamped to at least 4.5:1 on their track |

## 6. D-pad navigation register

Findings about the keypad experience itself, on top of how the selection is drawn.

| ID | Finding | Where | Proposed behaviour |
|---|---|---|---|
| `N-01` | Tappable controls with no d-pad path at all | WebConfig.qml (enable switch, URL toggle, PIN regenerate), NoPage.qml "+", NoProfile.qml button, NotificationDrawer.qml (brightness slider, list), ConnectionStatus.qml list, SearchField.qml, MainContainer.qml pull-down circles, Finish.qml URL toggle, About.qml links, Info popups close X, media Tv/Receiver browse/sources/shuffle/repeat | Every tappable element is reachable; touch-only extras need a d-pad equivalent on the same screen |
| `N-02` | Reachable but no visible selection | integrations/UserAction.qml:387 (focused Flickable), ManageEntities.qml tab bar, LoadingScreen.qml:355 Cancel, ProfileSwitch.qml:437 PIN Cancel, help-overlay/Base.qml:78 Close, Poweroff.qml Cancel (text colour only) | The ring is mandatory on anything that holds the focus |
| `N-03` | Selection highlights only the control, not the row: on a settings page the 2px border sits on a 90x60 switch at the far right while the label and help text stay unmarked | Display.qml, Ui.qml, Power.qml, Sound.qml, Voice.qml, Wifi.qml, SoftwareUpdate.qml | Accepted as designed (D-3): the ring stays on the control, now 3px in the focus colour; label and help text stay unmarked |
| `N-04` | Touch instructions shown to keypad users | docks/Info.qml:289 "Tap to edit name", onboarding/Start.qml:67 "Tap the screen to begin", Activity.qml:475 "Tap for more" | Wording that works for both inputs ("Press OK to edit") |
| `N-05` | Selection starts on the first row even when the screen was opened by touch (fixed in most places, still on in the four always-on sites) | entities/Base.qml:60, group/Base.qml:15, SelectWidget.qml:329, SourceList.qml:130 | Bind to `ui.keyNavigationActive` |
| `N-06` | Selected item can leave the viewport on scrolling pages without a scrollTarget; About pages scroll 100px per press; UserAction 200px | AboutPage.qml:18, LicensePage.qml, UserAction.qml | Half-viewport step; selected element kept inside the middle 70% (ListView already does this with the 15/85 range) |
| `N-07` | Destructive confirmations in bare text with a ring; Cancel invisible as a button (secondaryButton on black or on dark) | docks/Info.qml:816, integrations/Info.qml:702, WifiJoin.qml:245, the five rename dialogs | Secondary button variant with a visible outline; Cancel preselected |
| `N-08` | Menu rows have no navigation affordance; a d-pad user cannot tell a toggle row from a row that opens a page | Settings.qml, SettingsNew.qml, Profile.qml, About.qml | Rows that open a page end with a chevron; rows with a switch show the switch; rows with a value show the value |
| `N-09` | Reachable row that does nothing | docks/Info.qml:520 "Change WiFi settings" (Not implemented yet) | Hide until implemented |
| `N-10` | Dead scroll accelerator | settings/about/AboutPage.qml:18-59 (scrollCounter never increments) | Remove with the scroll-step rule |

## 7. Design system v2

Five rules, then the tokens and components that implement them. The intent is a phone-like settings experience on both panels without changing the identity of the product. This is v2: it follows the accepted decisions and the designer's revision of 2026-10-07.

1. **One palette for both panels.** Every token is chosen against a raised black, so what reads on the Remote 3 reads on the Remote Two. No per-model palette and no accessibility toggle in v1; both remain possible later because everything goes through tokens.

1. **Text meets AA at its size, with margin.** Secondary text at 8:1 on black (6:1 on the LCD estimate), primary at 13.6:1. Floor 22px, and 22px only for captions.

1. **Two selection styles, one per layer.** A lighter fill on the main UI, a 3px ring in settings. Never both on one element, and never a fill too dark to see on the LCD.

1. **Emphasis by token, never by opacity.** Opacity is reserved for disabled controls (0.4) and dim layers (0.85).

1. **Sans for reading, mono for data.** Poppins carries labels, prose, help and buttons. Space Mono is the value face: versions, addresses, times, codes, units.

### 7.1 Colour tokens

| Token | Value | On black | LCD est. | Replaces | Use |
|---|---|---|---|---|---|
| bg | #000000 | - | - | black | Page background and bottom sheets; content on a pressed fill. True black stays for the OLED |
| textPrimary | #D0D0D0 | 13.6:1 | 9.8:1 | offwhite | Labels, prose, values, icons; the pressed fill. Unchanged |
| textSecondary | #A0A0A0 | 8.0:1 | 6.0:1 | light #787878 | Help, captions, states, inactive icons |
| textDisabled | #7A7A7A | 4.9:1 | 3.7:1 | inactiveText #606060 | Only inside disabled controls |
| textOnButton | #FFFFFF | 6.9:1 on primary | 6.2:1 | offwhite on button | Primary and destructive button labels |
| surface | #1E1E1E | 1.26:1 | - | dark #161616, secondaryButton | Cards, fields, secondary buttons |
| surfaceRaised | #2C2C2C | 1.5:1 | - | medium #232323 | Popup cards over a dim, switch and slider tracks, tab bars |
| surfaceSelected | #595959 | 3.0:1 | 2.4:1 | new | Fill of the selected tile or menu row on the main UI, never with a ring. Provisional; the designer drew #333333 (1.66:1). Text on it is textPrimary (4.5:1) |
| divider | #3A3A3A | 1.85:1 | 1.6:1 | medium | 2px separators, secondary button outline, sheet edge, card outline |
| focusRing | #D0D0D0 | 13.6:1 | 9.8:1 | highlight #C8C8C8 | 3px ring on the selected settings row or control, and on buttons. Colour as in the revised screens (Q-2) |
| buttonPrimary | #5A5A5A | 3.0:1 | - | primaryButton | Primary button fill, white label |
| redPressed | #FF0D2A | - | - | new | Pressed destructive button; white label at 3.9:1, large text |
| red / green / orange / yellow | unchanged | 6.1 / 6.7 / 7.7 / 17 | ≥ 5 | same | Status and destructive. `blue` is retired from text use |

The palette is generated for the default black base colour. Only the Colors settings page, which is not in the menu, calls `generateColorPalette` with another colour; no entity screen tints the palette, so the token values above are the only ones in use.

### 7.2 Type roles

| Role | Face and size | Colour | Used for | Replaces |
|---|---|---|---|---|
| Title | Poppins 28 Medium | textPrimary | Title bar of pages, sheets, dialogs | Poppins 24/26/30 titles |
| Section heading | Poppins 26 Medium | textPrimary | "Known networks", "Added", drawer titles | Space Mono 24 light headers, Space Mono 28 drawer titles |
| Label / body | Poppins 30 Regular | textPrimary | Menu rows, setting labels, tile names, list rows | unchanged (100 sites) |
| Popup menu row | Poppins 28 Regular | textPrimary | Rows of a popup menu, including its Close row | unchanged from today's PopupMenu |
| Prose | Poppins 26 Regular, line height 1.3 | textPrimary | Release notes, legal, driver instructions, dialogs | Space Mono 24 light |
| Help / secondary | Poppins 26 Regular | textSecondary | Description under a setting, states, subtitles, key of a key/value row | Space Mono 24 light and six other combinations |
| Caption | Poppins 22 Regular | textSecondary | Timestamps, badges, tab labels. Smallest size allowed | 16 to 22px sites |
| Value | Space Mono 26 Regular | textPrimary | Versions, IP and MAC addresses, times, units, PIN, URLs | Space Mono 20/22 light |
| Button | Poppins 26 Medium | textOnButton / textPrimary | All three button variants | Space Mono 26 Bold, Poppins 30, bare captions |
| Display | Poppins 56 / 90 / 180 Light | textPrimary | Sensor values, entity states, volume | unchanged; Thin retired, Light only at 56 and above |

### 7.3 Layout

- **Gutter 20px** on every page, sheet and dialog. Content, dividers, fields and buttons align to it.

- **Selectable rows and tiles** are drawn from 8px to width-8 so the selection never touches the bezel; their content keeps the 20px gutter (12px inner padding).

- **Title bar 80px:** 80x80 back or close target at the edge, title centred, Poppins 28 Medium. One component for pages, sheets and dialogs. Onboarding uses the same bar without the back target.

- **Row heights** 80 (one line), 110 (two lines), 130 (tiles). Setting sections with help text size to content with 14px vertical padding.

- **Dividers** 2px `divider`, inset 20, between sections and rows that are not selectable cards.

- **Radii** 8 (rows, tiles, buttons, cards), 22 (fields, sheets, popups). No literals.

- **Icons** in boxes of 60 (row), 80 (bar, close), 100 (tile), glyph at half the box. Chevron at the end of every row that opens another screen.

- **Hit targets** at least 60x60; buttons 80 tall; a pair of buttons is two equal columns with a 20px gap inside the gutter.

- **Bottom sheets** are black with a 22px top radius and a 2px `divider` edge over a 0.85 black dim; the edge, not a fill, separates the sheet from the dimmed page on the LCD.

- **Popup menus end with a Close row** styled like the other rows of the menu, not a button.

### 7.4 Selection and state contract

Every selection is bound to `ui.keyNavigationActive`. The layer a screen belongs to decides the style.

| Style | Looks like | Used on |
|---|---|---|
| **Fill** | `surfaceSelected` fill on the whole tile or row; no ring, no border; everything on it in `textPrimary` | Main UI: entity and group tiles, popup menu rows (page menu and every PopupMenu), page selector, profile switcher |
| **Ring** | 3px `focusRing` around the selected element; no fill | Settings and set-up flows: settings and profile menus, settings rows and controls, picker lists, dock and integration lists, onboarding; buttons everywhere |

The tiles, rows and menus in the mockups are decided. The rest of the "Used on" column is the proposed mapping (Q-1).

- The ring goes on the element that OK activates: around a list row that opens a page or picks a value, on the control of a setting section with a switch, slider or field. The label and help text of such a section are not marked.

- Entering a screen by key preselects the first actionable element (`initialFocusItem` / selection reset, as documented in key-navigation.md). Entering by touch preselects nothing.

- The selected element stays inside the middle 70% of the viewport; the page scrolls by half its height when the chain ends.

- Pressed inverts the element: `textPrimary` fill with black labels, icons and chevrons (13.6:1). A secondary button keeps its outline; a pressed destructive button is `redPressed` with its white label.

- Disabled: opacity 0.4 on the whole control, switches and fields included; the row stays selectable so the user learns why it is off (help text), and OK does nothing.

- Current value (the selected option in a list of choices) is a check mark at the end of the row in `textPrimary`, never a selection style.

- Destructive confirmations start on Cancel; Cancel is the secondary variant with a visible outline.

- Every tappable element is reachable by d-pad, or has a d-pad equivalent on the same screen. Hints say "Press OK" or name the action, never "Tap".

### 7.5 Components to add or consolidate

**Components.Selectable.** Rectangle that draws the fill or the ring style from `selected`, gated on `ui.keyNavigationActive`; the host picks the style. Replaces RowHighlight and the seven inline patterns. Used by menu delegates, tiles, PopupList/PopupMenu rows, EntityList rows, cards, keypad keys.

**Components.TitleBar.** 80px bar with back or close target and Poppins 28 Medium title. Replaces TopNavigation, the SettingsNew/WebConfig header, the Setup wrappers, ManageEntities and dialog title bars.

**Components.SettingRow.** Label + control + optional help; the control carries the ring. Replaces the copied ColumnLayout/RowLayout blocks in Ui, Display, Power, Sound, Voice, Wifi, SoftwareUpdate, TouchSlider.

**Components.MenuRow.** Icon, label, trailing chevron or value or switch. Replaces the delegates in Settings, SettingsNew, Profile, About, PopupMenu, including the Close row of popup menus.

**Components.KeyValueRow.** Key in help style, value in value style, 2px divider. Replaces AboutInfo and the four hand-rolled variants.

**Components.Button variants.** `variant: primary | secondary | destructive`, Poppins 26 Medium, 80px. Retires bare text actions, Identify/Connect mini buttons, ButtonAdd's own style.

**Components.FormDialog.** Title bar, field, Cancel/OK inside the gutter. Replaces the nine rename/password dialogs.

**Components.Sheet.** Black bottom sheet with a 22px top radius, divider edge and 0.85 dim; hosts PopupMenu, WifiInfo, WifiJoin, DropDownMenu, delete drawers.

**Components.Prose.** Markdown/RichText body in prose style with the scroll indicator and the half-viewport d-pad step. Replaces ReleaseNotes, AboutPage, LicensePage, UserAction, Label.qml bodies.

**Fonts roles.** `fonts.title / label / menuRow / prose / help / caption / value / button / display(size)` in fonts.h, so QML stops passing pixel sizes. Existing calls keep working during the migration.

## 8. Screen mockups

The [mockups](mockups/README.md) hold a token and state sheet plus five screen pairs at 480 x 850, each drawn from the QML values (current) and from the rules above (proposed). Each mockup is a PNG plus a standalone HTML source; the source shows an approximation of the LCD (raised black, softer white) when opened with `?lcd`. The proposed screens carry the designer's revision of 2026-10-07 with the provisional #595959 fill.

| Pair | Shows |
|---|---|
| Settings menu | The invisible dark-fill selection versus a 3px light ring without fill; 80px title bar; chevrons on rows that open a page. |
| Display & Brightness | A 2px ring on the switch versus a 3px ring in the focus colour, still on the switch only; help text in Poppins 26 textSecondary; slider value shown as a mono value; 20px gutter and divider colour. |
| Release notes | Space Mono 24 light body versus Poppins 26 textPrimary prose with a version header, section heading and a visible scroll indicator. |
| Home page | The always-on #111 tile fill with a 1px border versus a lighter fill on the selected tile and no ring; state text in textSecondary at rest. |
| Page menu | The popup menu with dark-fill selection versus a black sheet with a visible edge, fill-selected rows and a Close row. |

## 9. Migration plan

Ordered so that the first step already fixes most of what you see daily, and each step is one reviewable pull request against remote-ui. Each phase starts as an OpenSpec change in remote-ui; the first one records the decisions in section 10 as ADRs. No step changes behaviour of the key handling documented in key-navigation.md.

| Phase | Change | Files | Effect | Risk |
|---|---|---|---|---|
| 1. Tokens | Retune the values in `Colors::generateColorPalette`: light → #A0A0A0, dark → #1E1E1E, medium → #2C2C2C, inactive → #7A7A7A, secondaryButton → surface with outline; add surfaceSelected (#595959), divider, textOnButton, redPressed, focusRing (#D0D0D0) and its 3px width. Old names stay as aliases. | src/ui/colors.cpp, colors.h | All secondary text, dividers, tracks and sheets lift at once, on both devices | Low: values only. Verify on a Remote 3 and a Remote Two in a dark room |
| 2. Selection | Add `Components.Selectable` with the fill and ring styles; replace the seven patterns (12 files in R-01 plus C, D, E, F, G sites); bind the four always-on sites to `ui.keyNavigationActive`; check the fill on both remotes (Q-3). | ~25 QML files | D-pad position visible on every screen, one look | Medium: touches delegates; test with UC_MODEL=UCR2 and on hardware |
| 3. Type roles | Add role functions to fonts.h; move help, prose, captions, values and buttons to their roles; enforce the 22px floor. | fonts.h, ~90 QML sites | Release notes, legal, help text, versions readable; consistent faces | Low visually, but long German and French strings need a layout check per screen |
| 4. Structure | TitleBar, SettingRow, MenuRow, KeyValueRow, FormDialog, Sheet, Prose components; normalise gutters, row heights, dividers, radii, icon sizes, button variants (I-03 to I-17). | settings, components, docks, integrations | Phone-like consistency; five copies become one | Medium: largest diff; do it screen family by family |
| 5. Reachability | Close the N-01 and N-02 gaps; wording of hints (N-04); hide the unimplemented row (N-09). | WebConfig, NoPage, NoProfile, NotificationDrawer, ConnectionStatus, SearchField, MainContainer, Info popups, UserAction, ManageEntities | Every screen fully operable from the keypad | Medium: each needs the idiom decision from key-navigation.md section 4 |
| 6. Guardrails | docs/design-system.md, a checklist item in key-navigation.md section 9, and a grep-based check in CI for pixel font sizes below 22, literal hex colours and Qt.lighter/darker in QML. | docs/, .github/workflows | New screens follow the rules without review effort | Low |

**Verification per phase.** Screenshots of the same six screens on a Remote 3 and a Remote Two, in a lit room and a dark room, before and after; a keypad walk of every settings page and every onboarding step after opening by key and by touch; the QML log free of binding loops and TypeErrors.

## 10. Decisions and open questions

Accepted on 2026-10-07. D-2, D-3 and D-5 follow the designer's revision rather than the original proposal.

| ID | Decision | Status |
|---|---|---|
| `D-1` | Help text and prose in Poppins; Space Mono stays the value face | accepted |
| `D-2` | Selection style: fill on the main UI, 3px ring in settings, never both | accepted, amended. Proposed: ring plus fill everywhere |
| `D-3` | In settings the ring marks the control, not the whole section | accepted, amended. Proposed: the whole section |
| `D-4` | Title bar 80px with Poppins 28 Medium | accepted |
| `D-5` | Home tiles black at rest; a selected tile gets the fill, no ring | accepted, amended. Proposed: ring plus fill. Fill value provisional (Q-3) |
| `D-6` | One palette for both panels | accepted |
| `D-7` | Structural phase covers settings, onboarding, docks and integrations first; entity detail screens later | accepted |

### The designer's revision

- Home page and page menu: the selected tile or row has a lighter fill and no ring.

- Settings menu: the selected row has a 3px light ring and no fill.

- Display & Brightness: the ring sits on the switch only.

- Page menu: a black sheet with its divider edge, and Close as the last menu row.

- Token sheet: pressed elements invert to a light fill with black content; a pressed destructive button is #FF0D2A; disabled switches and fields fade to 0.4.

The fill the designer drew is #333333 (1.66:1 on black, about 1.5:1 on the LCD estimate). It is set to #595959 for now, the lightest grey on which the primary text keeps 4.5:1, so the selection is easy to judge on a Remote 3. It can be darkened after the hardware check.

### Open questions

| ID | Question | Proposed answer |
|---|---|---|
| `Q-1` | Which screens beyond the mockups use the fill and which the ring? | The "Used on" column in 7.4 |
| `Q-2` | Ring colour: the revised screens use #D0D0D0, once at 92% opacity; the token sheet still shows #C8C8C8 | Opaque #D0D0D0, the primary text colour |
| `Q-3` | Is the provisional #595959 fill right on both remotes? | Check in phase 2; darken towards #333333 if it looks too heavy |
| `Q-4` | Do all bottom sheets become black with a divider edge, or only popup menus on the main UI? | All bottom sheets, one rule |

### Original proposal

The questions as they were put to review on 2026-09-16, kept for the rationale.

| ID | Question | Recommendation | Alternative |
|---|---|---|---|
| `D-1` | Secondary text face: move help and prose from Space Mono to Poppins? | Yes. Mono prose at 24px is the single biggest readability cost and the phone-like feel depends on it. Space Mono stays as the value face, so the identity survives. | Keep Space Mono for help text but at 26px in textSecondary. Fixes contrast, not legibility of long text. |
| `D-2` | Selected fill: ring plus `surfaceSelected`, or ring only? | Ring plus fill. The fill gives the shape at a glance, the ring guarantees it on the LCD. | Ring only, closer to today's buttons. Loses the row shape on the OLED in bright light. |
| `D-3` | Selection of a settings section: whole section or control only? | Whole section (label, control, help). | Control only with a stronger ring. Leaves the label unmarked on long pages. |
| `D-4` | Title bar 80px (from 60) with Poppins 28? | Yes. Matches row and tile rhythm and gives an 80px back target. | Keep 60px and only raise the title to 26 Medium. |
| `D-5` | Home tiles: keep black at rest, or give them a surface? | Keep black at rest (OLED look, battery), ring plus fill only when selected. | Surface tiles like a phone launcher. Larger visual change to the most-seen screen. |
| `D-6` | One palette or a per-panel palette? | One palette. Simpler, and the OLED loses nothing at these values. | A UCR3 branch in colors.cpp. Doubles the test matrix for a small gain. |
| `D-7` | Scope of phase 4: all screen families, or settings and onboarding first? | Settings, onboarding, docks and integrations first; entity detail screens (climate, covers, media) in a later cycle, since they have no on-screen selection and their issues are colour only. | Everything at once. |

## 11. Appendix

### A. Contrast method

WCAG 2.x relative luminance from sRGB; ratio = (L1 + 0.05) / (L2 + 0.05). The LCD estimate maps every luminance L to L × 0.98 + 0.02 before computing the ratio (a 2% black level). The `?lcd` view of the mockups uses `filter: contrast(0.86)`, which raises black to about 7% and lowers white to 93%; it is a visual aid, not the same model.

### B. Palette generation today

`Colors::generateColorPalette` derives dark, medium, light and highlight from the hue of the base colour with fixed HSV saturation and value: dark (h, 200, 22), medium (h, 200, 35), light (h, 40, 120), highlight (h, 160, 200). With the default black base the hue is undefined and the result is grey. The Colors settings page that could change the base is disabled in the menu.

### C. What was read

Settings: 32 files (settings/, settings/settings/, about/, softwareupdate/, docks/, integrations/ incl. fields/, SettingsNew, AboutInfo, WebConfig). Components: 50 files (components/ root, group/, help-overlay/, keypad/, keyboard style, MainContainer, NoPage, NoProfile, OnboardingContainer). Onboarding and entities: 13 onboarding steps, 12 entity building blocks, 34 device-class screens. C++: colors.cpp/h, fonts.h, uiController.cpp/h, main.cpp. Docs: key-navigation.md, CLAUDE.md, CHANGELOG.md.

### D. Companion documents

- [Design system](../design-system.md): the guideline for building a new screen, the authoritative text for implementation.
- [Mockups](mockups/README.md): the token and state sheet and the five screen pairs, as PNG renders and
  standalone HTML sources.
