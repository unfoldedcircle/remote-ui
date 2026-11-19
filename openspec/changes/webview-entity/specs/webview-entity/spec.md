## Purpose

The web view entity of the Core-API: a web page the UI shows on a profile page tile and fullscreen, and whose
commands the UI executes for the core, with the entity's options, its secret credentials and the global web view
settings.

## ADDED Requirements

### Requirement: Web view entity type
The UI SHALL support the entity type `webview` with the state attribute values `ON`, `UNAVAILABLE` and `UNKNOWN`. A
web view without an icon SHALL get the default icon `uc:globe`. Its tile SHALL show the icon and the name; the state
text only when the web view is not `ON`. A tap on the tile or on its icon SHALL open the fullscreen web view; a tap on
an unavailable web view SHALL be refused like for every other entity.

#### Scenario: Web view without icon
- **WHEN** the core delivers a web view entity without an `icon`
- **THEN** its tile shows the `uc:globe` icon and the entity's name

#### Scenario: Tap on the tile
- **WHEN** the user taps the tile of a web view that is `ON`
- **THEN** the fullscreen web view opens with its start page

### Requirement: Web view options and user overrides
The UI SHALL apply a web view's options: the start page `url` (http or https only, without user name and password),
`zoom` (25..400 %), `background_color`, `show_navigation`, `reload_interval` (0 or 10..86400 s), `user_agent` and
`buttons`. The user overrides `zoom`, `reload_interval`, `show_navigation` and `buttons` (button by button) in
`options.user` SHALL take precedence; a `null` override SHALL be ignored. An invalid value SHALL be ignored and the
default used (ADR 0018).

#### Scenario: Zoom override
- **WHEN** a web view from an integration has the option `zoom` 70 and the user override `zoom` 60
- **THEN** its page is shown at 60 %

#### Scenario: Address with credentials
- **WHEN** a web view's `url` is `https://user:secret@example.com/`
- **THEN** the UI treats it as a web view without a start page and shows that the page has no valid address

### Requirement: Secret options of a web view
The UI SHALL take the secret options of a web view (`auth.password`, `auth.token`, `auth.header_value`,
`cookies[].value`) only from the core's answers to its own session. An `entity_change` event whose options flag a
secret without its value SHALL NOT be applied; the UI SHALL fetch the entity instead and apply only the answer of its
latest fetch. The UI SHALL never log or display a secret.

#### Scenario: Rotated token
- **WHEN** an `entity_change` event of a web view carries `auth.has_token` without the token
- **THEN** the web view keeps its current token, the UI sends `get_entity` and applies the token of the answer

#### Scenario: Options without secrets
- **WHEN** an `entity_change` event changes the zoom of a web view that has no secret option
- **THEN** the new zoom is applied at once

### Requirement: Declaration to the core
After every authentication the UI SHALL send `set_ui_info` with its version. The `entity_types` SHALL contain
`webview` when the web view library is installed, also before any web view entity is known; the UI SHALL NOT load the
library for that. Once its first use loaded the library, `webview` SHALL be declared only while that load and the
initialisation succeeded, and a change SHALL be sent at once.

#### Scenario: Remote with the web view library
- **WHEN** the UI authenticates on a remote whose firmware ships the web view library
- **THEN** it sends `set_ui_info` with `entity_types` `["webview"]` without loading the library, before the web view
  entities are loaded, and the core reports a web view `ON` from its creation

#### Scenario: Library that fails at its first use
- **WHEN** the library is installed but does not load when the core sends the first `open` command
- **THEN** the UI answers the command 503 and sends `set_ui_info` with an empty `entity_types` list, and the core
  reports its web views `UNAVAILABLE`

#### Scenario: Simulator
- **WHEN** the desktop simulator, which has no web view library, authenticates
- **THEN** it sends `set_ui_info` with an empty `entity_types` list and the core keeps web views `UNAVAILABLE`

### Requirement: Web view commands from the core
The UI SHALL answer every `entity_command` request of the core exactly once with a `result` under the request's id: 400
for another entity type or an unknown command, 404 for an unknown web view, 503 when the UI cannot show web pages, at
once. `open` SHALL open the fullscreen web view and `reload` SHALL reload the page of that web view where it is shown
and do nothing otherwise; both SHALL be answered 200 at once.

#### Scenario: Open from another system
- **WHEN** the core sends `entity_command` with `cmd_id` `open` for a configured web view
- **THEN** the UI answers 200 and shows the web view fullscreen

#### Scenario: Unknown web view
- **WHEN** the core sends `entity_command` for an entity id the UI does not know
- **THEN** the UI answers 404 and nothing opens

### Requirement: Clearing website data
`clear_cache` SHALL clear the HTTP cache and `clear_data` all website data of every web view. Both SHALL be answered
when the web view library reports the end of the clear: 200, after which every shown page reloads, or 500 when the
library refused the request or reports a failure. Only when the firmware's web view library cannot clear website data
SHALL they be answered with 501.

#### Scenario: Clear the cache
- **WHEN** the core sends `clear_cache` for a web view and the library reports the end of the clear
- **THEN** the UI answers 200 after the report and the shown page reloads

#### Scenario: The clear fails
- **WHEN** the core sends `clear_data` and the library reports a failure
- **THEN** the UI answers 500 and no page is reloaded

#### Scenario: Clear the data of a signed-in site
- **WHEN** the user signed in to a site in a login dialog and the core sends `clear_data`
- **THEN** cookies, storage and caches are cleared and the page reloads, and the site's HTTP login stays valid until
  the UI restarts, because the web engine keeps the credentials of answered logins

### Requirement: Opening the fullscreen web view
The fullscreen web view SHALL open on top of the current screen; closing it SHALL return to that screen. While an
activity's loading screen is shown, the web view SHALL open after it, on top of the activity screen. An open
fullscreen web view SHALL stay as it is for the same web view and show the other page for another web view.

#### Scenario: Open as a step of an activity's on-sequence
- **WHEN** an activity starts and its on-sequence contains `webview.open`
- **THEN** the web view opens after the loading screen, on top of the activity screen, and BACK returns to the activity

#### Scenario: Open while the same web view is shown
- **WHEN** `open` arrives for the web view that is shown fullscreen
- **THEN** the page stays as it is, without a reload

### Requirement: Fullscreen web view keys and controls
The fullscreen web view SHALL cover the screen. With `show_navigation` it SHALL show Back, Forward, Reload and Close.
HOME SHALL close it and the screen it was opened on; POWER, VOICE and every button the button mapping does not name
SHALL keep the button mapping of the screen below. Every other button of the remote SHALL have the action of the
button mapping of the fullscreen web view.

#### Scenario: Volume on top of an activity
- **WHEN** a web view is shown fullscreen on top of an activity whose VOLUME_UP is mapped to its receiver, and neither
  the web view nor the global setting changes VOLUME_UP
- **THEN** VOLUME_UP zooms the page one level and the receiver's volume does not change

#### Scenario: Other buttons on top of an activity
- **WHEN** a web view is shown fullscreen on top of an activity whose MUTE is mapped to its receiver
- **THEN** MUTE mutes the receiver

#### Scenario: HOME
- **WHEN** the user presses HOME in a fullscreen web view opened on top of an activity
- **THEN** the web view and the activity screen close and the home screen is shown

### Requirement: Buttons of the button mapping
The button mapping of the fullscreen web view SHALL name the buttons of the remote model: BACK, DPAD_UP, DPAD_DOWN,
DPAD_LEFT, DPAD_RIGHT, DPAD_MIDDLE, VOLUME_UP, VOLUME_DOWN, CHANNEL_UP, CHANNEL_DOWN, MUTE, PREV, PLAY and NEXT, and
STOP, RECORD and MENU on the Remote 3, RED, GREEN, YELLOW and BLUE on the Remote Two and in the desktop simulator. A
button of the other model SHALL NOT be mapped.

#### Scenario: A button of the other model
- **WHEN** the global setting maps RED to the key `ColorF0Red` and a Remote 3 shows a web view on top of an activity
  that maps RED
- **THEN** RED keeps the activity's mapping

### Requirement: Action of a button
The action of a button SHALL be the first of: the web view's user override (`options.user.buttons`), the web view's
`buttons` option, the global web view setting `buttons`, the default action. An unknown button or an unknown action,
function or key SHALL be ignored, and the next source SHALL apply to that button (ADR 0018). A change SHALL apply at
once, also to an open web view.

#### Scenario: The web view's own buttons
- **WHEN** the global setting maps VOLUME_UP to `pass`, a web view's `buttons` option maps it to `zoom_in`, and the user
  override of that web view maps CHANNEL_UP to `pass` over its option's key `ChannelUp`
- **THEN** in that web view VOLUME_UP zooms the page and CHANNEL_UP keeps the activity's mapping

#### Scenario: A change while the web view is open
- **WHEN** the global setting of VOLUME_UP changes to `pass` while a web view is shown fullscreen on top of an activity
- **THEN** the next VOLUME_UP raises the receiver's volume and the page is not zoomed

### Requirement: Default actions of the buttons
Without an action of the web view or of the global setting, the d-pad SHALL send the keys `ArrowUp`, `ArrowDown`,
`ArrowLeft` and `ArrowRight`, DPAD_MIDDLE the key `Enter`, MENU the key `ContextMenu`; BACK SHALL have the function
`close`, VOLUME up and down `zoom_in` and `zoom_out`, CHANNEL up and down `page_up` and `page_down`, and every other
button `pass`.

#### Scenario: The d-pad by default
- **WHEN** the user presses DPAD_DOWN in a fullscreen web view
- **THEN** the page receives a keydown and a keyup with `key` and `code` `ArrowDown`

### Requirement: Key action of a button
A button with a `key` action SHALL send a key event to the page with that `KeyboardEvent.key` and its matching `code`,
pressed at the button's press and released at its release.

#### Scenario: A key for a TV page
- **WHEN** CHANNEL_UP is mapped to the key `ChannelUp` and the user presses it in a fullscreen web view
- **THEN** the page receives a keydown and a keyup with `key` `ChannelUp`, and the page does not scroll

### Requirement: Function action of a button
A button with a `function` action SHALL run the web view function: `zoom_in`, `zoom_out`, `zoom_reset`; `scroll_up` and
`scroll_down` a small step; `page_up` and `page_down` a screen; `top` and `bottom`; `back` and `forward` when there is
history; `back_or_close`, which closes on the first page; `reload`, `stop`, `close`; `toggle_navigation`, the navigation
bar for the visit; `focus_next` and `focus_previous`, like Tab and Shift+Tab; `none` nothing.

#### Scenario: Back or close on the first page
- **WHEN** BACK is mapped to `back_or_close` and the fullscreen web view shows its start page without history
- **THEN** BACK closes the web view

#### Scenario: Toggle the navigation
- **WHEN** a button with `toggle_navigation` is pressed in a web view without `show_navigation`
- **THEN** the navigation bar shows, and the entity's `show_navigation` stays as it is

### Requirement: Pass action of a button
A button with `pass` SHALL keep its function in the layer below: on top of an activity its button mapping, opened from
a profile page what the page header's activity or media player does with it. BACK without such a function SHALL close
the web view.

#### Scenario: Pass on a profile page
- **WHEN** a web view is opened from a tile of a profile page whose header shows a playing media player, and PLAY has
  the default `pass`
- **THEN** PLAY pauses that media player

### Requirement: Held buttons in the fullscreen web view
A held button SHALL repeat key events and the functions `scroll_up`, `scroll_down`, `page_up` and `page_down`; every
other function SHALL act once per press, and a `pass` button SHALL repeat as in the layer below. A held BACK whose
action is not `close` SHALL NOT close the web view.

#### Scenario: Held d-pad
- **WHEN** the user holds DPAD_DOWN in a fullscreen web view
- **THEN** the page receives repeated keydowns of `ArrowDown` and one keyup at the release

#### Scenario: Held VOLUME
- **WHEN** the user holds VOLUME_UP with its default `zoom_in`
- **THEN** the page is zoomed one level

### Requirement: The d-pad while typing in the fullscreen web view
While the on-screen keyboard is up, the d-pad and OK SHALL go to the text field of the page as arrow keys and Enter,
whatever their action. Every press of a button SHALL reach the page at most once. While a dialog or login of the page
is open, the buttons SHALL be its own. While a failed load shows its message, OK SHALL load the page again.

#### Scenario: Typing with OK mapped to reload
- **WHEN** DPAD_MIDDLE is mapped to `reload` and the user presses OK while the on-screen keyboard is up on a field of
  the page
- **THEN** the page receives Enter once and is not reloaded

#### Scenario: A dialog of the page
- **WHEN** an `alert()` of the page is open and the user presses CHANNEL_UP mapped to the key `ChannelUp`
- **THEN** the page receives no key event

### Requirement: Zoom functions of the fullscreen web view
`zoom_in` and `zoom_out`, by default on VOLUME up and down, SHALL zoom the page one level in or out per press, through
the levels 50 to 300 %, starting at the entity's effective zoom; `zoom_reset` SHALL go back to it. The level SHALL last
while the page is shown and SHALL NOT change the entity's options. Nothing SHALL be zoomed while a dialog of the page or
the on-screen keyboard is up.

#### Scenario: Zoom from the entity's zoom
- **WHEN** a web view with the zoom 60 % is shown fullscreen and the user presses VOLUME_UP
- **THEN** the page is shown at 67 % and a label at the top shows "67 %"

#### Scenario: Zoom for the visit only
- **WHEN** the user zooms a fullscreen web view with VOLUME, closes it and opens it again
- **THEN** the page is shown at the entity's zoom, and the entity's `zoom` option is unchanged

#### Scenario: Last level
- **WHEN** the page is shown at 300 % and the user presses VOLUME_UP
- **THEN** the zoom stays at 300 % and the label shows "300 %"

#### Scenario: Reset
- **WHEN** a web view with the zoom 80 % is zoomed to 100 % and a button with `zoom_reset` is pressed
- **THEN** the page is shown at 80 % again

### Requirement: The page of a web view
The page SHALL be loaded when the web view is shown and unloaded when it is closed. A change of the start page or of
the credentials, cookies, TLS exception or user agent SHALL load the start page again; a change of zoom or background
colour SHALL apply to the page as it is. The reload interval SHALL run only while the page is shown and the display is
on. Without `background_color` the background SHALL be white `#FFFFFF`, so that a page without a background of its
own stays readable.

#### Scenario: New start page
- **WHEN** the start page of a web view changes while it is shown
- **THEN** the new start page is loaded

#### Scenario: Display off
- **WHEN** the display turns off while a web view with a reload interval of 60 s is shown
- **THEN** the page is not reloaded until the display is on again

#### Scenario: Before the first frame
- **WHEN** a web view without `background_color` opens and its page has not painted yet
- **THEN** the page's area is white, and nothing of the screen behind shows

### Requirement: Load indicator of a web view
While the page of a web view loads, the fullscreen web view and the web view widget SHALL show nothing for the first
400 ms of the load. Then a new page SHALL show a centred spinner with the load progress until its first frame after the
load committed, and then a thin bar with the load progress at the top edge until it is loaded. A reload SHALL show only
the bar, a reload of the reload interval nothing. Neither SHALL take a touch or a key of the page.

#### Scenario: Slow page
- **WHEN** the fullscreen web view opens and the start page's server answers after 3 s
- **THEN** nothing shows for 400 ms, then the spinner with the percentage loaded until the page's first frame, then
  the bar at the top until the page is loaded

#### Scenario: Quick page
- **WHEN** the page shows its first frame and is loaded within 400 ms
- **THEN** no load indicator shows

#### Scenario: Reload
- **WHEN** the user presses Reload in the navigation bar, or the core sends `reload`, and the page takes 2 s
- **THEN** only the bar shows over the page until it is loaded

#### Scenario: Reload interval
- **WHEN** the reload interval reloads the page
- **THEN** no load indicator shows

#### Scenario: Back to a slow page
- **WHEN** the user presses Back in the navigation bar and the previous page takes 2 s
- **THEN** the spinner shows after 400 ms until that page's first frame

#### Scenario: Touch while the bar shows
- **WHEN** the bar shows over a page that loads on and the user taps a link
- **THEN** the tap reaches the page

### Requirement: Failed load of a web view
A failed load SHALL show a short message over the page's background instead of the web engine's error page, in the
fullscreen web view with "Press OK to try again", until a load starts that the user waits for. OK and a tap on the
message SHALL load the page again. A second crash of the web process, which the UI does not reload, SHALL show the
failure as well.

#### Scenario: Failed load
- **WHEN** the host of the start page cannot be reached
- **THEN** "The page could not be loaded" and "Press OK to try again" show at once over the page's background, and
  neither the address nor the web engine's error text shows

#### Scenario: Try again
- **WHEN** the failure shows, the server is reachable again and the user presses OK
- **THEN** the message goes, the spinner shows after 400 ms and the page appears

### Requirement: Configured credentials
With `auth.type` `basic`, the UI SHALL answer an HTTP Basic or Digest login of the start page's origin (scheme, host
and port) with the configured user name and password, without a dialog. It SHALL NOT answer a login of another origin
or of a proxy with them, and SHALL show the login dialog when the server rejected them.

#### Scenario: Protected status page
- **WHEN** the start page's server asks for a Basic login
- **THEN** the configured credentials are sent and the page loads without a dialog

#### Scenario: Rejected credentials
- **WHEN** the server rejects the configured credentials
- **THEN** the login dialog opens with the configured user name filled in

### Requirement: Site settings of a web view
With a web view library that has the functions (library 0.8.0) the UI SHALL apply, before the page's first request and
again when they change: the request header of `bearer` and `header` authentication to the requests to the start
page's origin, the user agent of the view, and the TLS exception for the start page's host. A certificate exception
SHALL stay for the life of the UI process: the library cannot remove it.

#### Scenario: Bearer token
- **WHEN** a web view with `auth.type` `bearer` is shown
- **THEN** the page, its resources and its `fetch()` calls to the start page's origin carry `Authorization: Bearer
  <token>`, and a request to another origin does not

#### Scenario: Self-signed certificate
- **WHEN** a web view's start page has a self-signed certificate whose SHA-256 fingerprint is the `tls` option's
- **THEN** the page loads instead of the certificate error page

#### Scenario: Removed certificate exception
- **WHEN** the `tls` option of a shown web view is removed
- **THEN** the certificate stays accepted until the UI restarts

### Requirement: Cookies before the start page
With a web view library that can set cookies (library 0.8.0) the UI SHALL set a web view's cookies before every load of
an address of the start page's origin through the item's address, and load the page when the library reports them.
When the library refuses them, reports a failure or does not report within 5 s, the page SHALL load without them. A
reload SHALL NOT set the cookies again, except the first reload after all website data was cleared.

#### Scenario: Cookies before the start page
- **WHEN** a web view with a session cookie opens
- **THEN** the UI sets the cookie, and the start page loads once the library reported it

#### Scenario: The cookies are not confirmed
- **WHEN** the library does not report the cookies within 5 s
- **THEN** the start page loads and the log says that the cookies were not confirmed, without their values

#### Scenario: Cookies after clear_data
- **WHEN** the core sends `clear_data` while a web view with configured cookies is shown
- **THEN** the cookies are set again before the page reloads

### Requirement: Older web view library
With a web view library older than 0.8.0 the UI SHALL load the page without what that library cannot apply: the
request header of `bearer` and `header` authentication, cookies, the TLS exception, a user agent other than `default`,
the `dark` colour scheme and invalid certificates of all hosts. It SHALL log which options are ignored, by name,
without their values, and answer `clear_cache` and `clear_data` with 501.

#### Scenario: Bearer token with an older library
- **WHEN** a web view with `auth.type` `bearer` is shown with a library that cannot add request headers
- **THEN** the page loads without the header and the log names `auth.bearer` as ignored, without the token

#### Scenario: Clear the cache with an older library
- **WHEN** the core sends `clear_cache` and the library cannot clear website data
- **THEN** the UI answers 501 and no page is reloaded

### Requirement: Global web view settings
The UI SHALL request `get_webview_cfg` after every authentication and apply the `webview` object of every
`configuration_change` event. `color_scheme` and `allow_invalid_certificates` SHALL be passed to the web view library
once it is initialised and on every change, never as a reason to load it. `website_data` SHALL be applied when a web
view closes: the fullscreen web view, or the activity page with a web view widget is left. A value the UI does not know
SHALL keep the default (ADR 0018).

#### Scenario: Dark colour scheme
- **WHEN** the core reports `color_scheme` `dark` while a page with a `prefers-color-scheme` theme is shown
- **THEN** the page switches to its dark theme without a reload

#### Scenario: Clear on close
- **WHEN** `website_data` is `clear_all_on_close` and the user closes the fullscreen web view
- **THEN** the UI asks the web view library to clear all website data

#### Scenario: Unknown colour scheme
- **WHEN** the core reports a `color_scheme` the UI does not know
- **THEN** the UI keeps the `dark` default

#### Scenario: Colour scheme changed on the remote
- **WHEN** the user picks "Light" in Settings > Web view and the core accepts it
- **THEN** the web view library gets the light colour scheme and pages with a `prefers-color-scheme` theme switch to it
