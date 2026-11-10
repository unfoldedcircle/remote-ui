## Purpose

Shows a web page inside the user interface with the web engine the firmware provides: the web view and
its developer page in the settings, the page's input by touch, the keypad and the on-screen keyboard,
the page's dialogs and logins, and what the web view writes to the log.

## ADDED Requirements

### Requirement: The web view needs the firmware's library
The web view SHALL be available only when the firmware provides its web view library with the API
version the UI was built for. Otherwise the UI SHALL start and work as before: the settings list no
"Web view" entry, a web view placed on a screen shows nothing and reports itself unavailable, and the
reason is logged once.

#### Scenario: Firmware without the library
- **WHEN** the UI runs on a firmware that does not ship the web view library
- **THEN** the Settings menu has no "Web view" entry and every other screen works as before

#### Scenario: Library of another API version
- **WHEN** the firmware's web view library reports another API version than the UI was built for
- **THEN** the web view is unavailable and one log line names both versions

#### Scenario: Older library of the same API version
- **WHEN** the library is an older release of the same API version
- **THEN** pages load, and only the features of later library releases are missing, each named once
  in the log: for example no pinch zoom before library 0.6.0 and no dialogs before 0.7.0

### Requirement: Developer page
The "Web view" page SHALL show an address field with a Load button, below it Back, Forward and
Stop/Reload buttons next to a status line, and the web page below them. The status line SHALL show the
page title or the load progress, the counters of received and released frames, and the last crash or
failed load. Stop/Reload SHALL show a cross that stops a running load, otherwise a circular arrow that
reloads the page.

#### Scenario: Loading an address
- **WHEN** the user types `example.com` into the address field and taps Load
- **THEN** `http://example.com` loads, the keyboard goes down, and the status line shows the progress
  and then the page title

#### Scenario: Stop and reload
- **WHEN** a slow page is loading and the user taps Stop/Reload
- **THEN** the load stops and the button changes to the circular arrow, which reloads the page

### Requirement: History buttons of the developer page
Back and Forward SHALL go one step back and forward in the page's history. Without history in their
direction they SHALL be dimmed, ignore taps and be skipped by the d-pad. When the button under the
d-pad selection becomes disabled, the selection SHALL move to the other history button or to
Stop/Reload. History steps inside a single-page application SHALL enable the buttons as well.

#### Scenario: After following a link
- **WHEN** the user follows a link of the first page
- **THEN** Back is enabled, and Back returns to the first page, dims itself and enables Forward

#### Scenario: Back turns disabled under the selection
- **WHEN** the d-pad selection is on Back and Back reaches the first page
- **THEN** the selection moves to Forward, and the d-pad keeps working

### Requirement: Address field of the developer page
An address typed into the field SHALL be completed: an address with an allowed scheme stays as it is,
an absolute path becomes a `file:` address, and anything else gets `http://`. OK on the field SHALL
load like Load. Except while the keyboard is up on it, the field SHALL show the address of the page on
screen, also after a redirect, Back or Forward; a refused address SHALL stay in the field.

#### Scenario: Host with a port
- **WHEN** the user loads `localhost:8080/x`
- **THEN** `http://localhost:8080/x` is loaded

#### Scenario: Redirected page
- **WHEN** a loaded address redirects to another one
- **THEN** the field shows the address the page was redirected to

### Requirement: Keypad walk of the developer page
With the keypad the page SHALL open with the selection on Load. LEFT and RIGHT SHALL move between the
address field and Load, the field handing them on only at the start and end of its text. DOWN SHALL
lead to the first enabled button of the second row, from Load to Stop/Reload, and on into the web view;
UP from a button of the second row SHALL return to the address field. The keyboard SHALL come up while
the selection is on the address field.

#### Scenario: Walking the controls
- **WHEN** the page is opened with the keypad and the user presses LEFT, RIGHT, DOWN and DOWN
- **THEN** the selection goes to the address field with the keyboard up, back to Load with the keyboard
  down, to Stop/Reload while Back and Forward are disabled, and into the web view

### Requirement: Allowed addresses
The web view SHALL load only `http`, `https`, `about` and `data` addresses, and `file` addresses only
when the UI runs with `UC_WEBVIEW_ALLOW_FILE_URLS=1`. Any other address SHALL NOT be loaded: the
address shown stays, the status line reports a failed load, and a warning without the address's
secrets is logged.

#### Scenario: File address refused
- **WHEN** the user loads `/app/data/page.html` without `UC_WEBVIEW_ALLOW_FILE_URLS=1`
- **THEN** nothing loads, the status line shows `load failed: file:///app/data/page.html` and the
  log says that `file:` addresses are disabled

#### Scenario: Script address
- **WHEN** a `javascript:` address is given to the web view
- **THEN** it is not loaded and a failed load is reported

### Requirement: Page background
Where a page paints no background of its own, the web view SHALL show white, as a desktop browser
does.

#### Scenario: Page without a background
- **WHEN** a page without a background colour of its own is loaded
- **THEN** its black text is shown on white

### Requirement: Touch in the page
A press and release in the page SHALL be a click as long as the finger moves no more than the drag
threshold, 16 px with the firmware's library from 0.3.0; a longer move SHALL scroll the page. A touch
in the web view SHALL reach only the page. A touch sequence that another control takes over SHALL be
cancelled in the page, so that later taps still work.

#### Scenario: Tap with a rolling finger
- **WHEN** the user taps a link and the finger moves 12 px before it lifts
- **THEN** the link opens

#### Scenario: Drag
- **WHEN** the finger moves 40 px before it lifts
- **THEN** the page scrolls and nothing is clicked

### Requirement: Keypad in the page
While the d-pad selection is in the web view, the arrows and OK SHALL go to the page, which scrolls
or uses them for its own navigation and forms, and the web view SHALL show the selection ring of the
design system. The selection SHALL leave the web view only through a tap on another control or with
BACK, which leaves the page. The remote's other buttons SHALL NOT be passed to the page as keys of
their own.

#### Scenario: Scrolling with the d-pad
- **WHEN** the selection moves down into the web view and the user presses DPAD_DOWN
- **THEN** the ring surrounds the web view and the page scrolls down

#### Scenario: Colour key in the page
- **WHEN** the selection is in the web view and the user presses RED
- **THEN** the page receives no key

### Requirement: History keys of the developer page
On the developer page PREV and NEXT SHALL go one step back and forward in the page's history,
wherever the d-pad selection is, and SHALL do nothing without history in that direction. A held key
SHALL take one step only. BACK SHALL leave the page and SHALL NOT go back in the history.

#### Scenario: PREV inside the web view
- **WHEN** the selection is in the web view after two followed links and the user presses PREV
- **THEN** the previous page is shown and the selection stays in the web view

#### Scenario: Held PREV
- **WHEN** the user holds PREV for two seconds
- **THEN** the page goes back one step, not several

### Requirement: Zoom keys of the developer page
On the developer page VOLUME + and - SHALL zoom the page one level in or out, wherever the selection
is, through 50, 67, 75, 80, 90, 100, 110, 125, 150, 175, 200, 250 and 300 %, laying the page out anew.
A held key SHALL change one level only, and the zoom SHALL stop at the first and last level. The zoom
SHALL last for the visit of the page, across links, history steps and reloads.

#### Scenario: Zoom in
- **WHEN** a text page is shown at 100 % and the user presses VOLUME +
- **THEN** the text grows to 110 % and wraps anew

#### Scenario: Last level
- **WHEN** the page is at 300 % and the user presses VOLUME +
- **THEN** the page stays at 300 %

#### Scenario: Next visit
- **WHEN** the user leaves the page with BACK and opens it again
- **THEN** pages are shown at 100 %

### Requirement: Zoom level label
After every zoom key press a label with the level SHALL appear at the top of the web view. It SHALL
stay while the page renders the level and for about a second after the last press, then fade out. A
render that takes longer than 150 ms SHALL add the app's spinner next to the level. Presses during a
render SHALL update the label at once, and the page SHALL then render only the latest level.

#### Scenario: Quick presses on a large page
- **WHEN** the user presses VOLUME + three times quickly on a large page at 100 %
- **THEN** the label counts up to "150 %" at once, the spinner turns until the page shows 150 %, and
  the page is laid out at most twice

### Requirement: Page scroll keys of the developer page
On the developer page CHANNEL - and + SHALL scroll the page a screen down and up, wherever the
selection is, as the Page Down and Page Up keys of a keyboard do: the page may handle them itself, and
a scrolling part of the page that holds the focus scrolls first. A held key SHALL keep scrolling.

#### Scenario: Long article
- **WHEN** a long article is shown and the user presses CHANNEL -
- **THEN** the page scrolls about a screen down and the selection stays where it was

#### Scenario: Scrolling element
- **WHEN** the user tapped into a scrolling `<div>` of the page and presses CHANNEL -
- **THEN** the `<div>` scrolls

### Requirement: Page keys wait while the user types
PREV, NEXT, VOLUME and CHANNEL SHALL do nothing on the developer page while the on-screen keyboard is
up, on the address field or on a field of the page.

#### Scenario: Typing into the page
- **WHEN** the keyboard is up for a text field of the page and the user presses PREV or VOLUME +
- **THEN** the page neither goes back nor zooms, and no zoom label appears

### Requirement: Pinch zoom
On a touch panel that reports two fingers, of the supported models only the Remote Two, two fingers
moving apart or together SHALL magnify the page between 1× and 4× around the point where the pinch
began, with the magnification in the zoom label, and the page SHALL turn sharp once it has rendered
it. A new address or a followed link SHALL start at 1×; a history step SHALL restore the page's
earlier magnification.

#### Scenario: Pinch on the Remote Two
- **WHEN** the user spreads two fingers on a word of a text page
- **THEN** the page grows around the word, the label shows the magnification, and after the fingers
  lift the word stays in place and turns sharp

#### Scenario: Remote 3
- **WHEN** the user puts two fingers on the page on a Remote 3, whose panel reports one finger
- **THEN** the first finger taps or scrolls as usual and the page zooms with VOLUME only

#### Scenario: Page with a pinch of its own
- **WHEN** the user pinches a map page that handles two fingers itself
- **THEN** the map zooms and the web view does not magnify the page

### Requirement: On-screen keyboard for the page
When the user selects an editable element of the page, the on-screen keyboard SHALL come up with the
layout and hints its input type asks for, such as digits, an address, an e-mail address or hidden
text, and typed text, corrections and deletions SHALL reach the element. The keyboard SHALL follow the
focus owner: when the user selects the address field next to the page, the keyboard SHALL stay up for
the field without closing and opening again.

#### Scenario: Numeric field
- **WHEN** the user taps an `<input inputmode="numeric">` of the page
- **THEN** the keyboard comes up with digits and the typed digits appear in the field

#### Scenario: From the page to the address field
- **WHEN** the keyboard is up for a field of the page and the user taps the address field
- **THEN** the keyboard stays up for the address field and does not close and reopen

### Requirement: Keyboard back after its hide key
A tap on the page's field that still has the focus SHALL bring the keyboard back after its hide key
closed it, within about 0.3 s. A tap on a button or on empty space of the page SHALL NOT bring the
keyboard up, not even for a moment.

#### Scenario: Same field again
- **WHEN** the user closes the keyboard with its hide key and taps the same field of the page
- **THEN** the keyboard comes back

#### Scenario: Tap on a button
- **WHEN** the keyboard was closed with its hide key and the user taps a button of the page
- **THEN** the keyboard stays down

### Requirement: Fields the page selects by itself
A field that the page selected by itself, such as an autofocused PIN field, SHALL NOT open the
keyboard until the user taps it, and a tap SHALL then open it. A field marked `inputmode="none"` that
the user selected SHALL never open the keyboard, also on later taps.

#### Scenario: Autofocused PIN field
- **WHEN** the web configurator's login page selects its PIN field by itself and the user taps it
- **THEN** no keyboard comes up before the tap, and the keyboard comes up after it

#### Scenario: Page with its own keypad
- **WHEN** the user taps an `<input inputmode="none">` twice
- **THEN** the keyboard does not come up

### Requirement: Animation frames at the display rate
With the firmware's library from 0.2.0 the web view SHALL run the page's animation frames at the
refresh rate of the display, 50 Hz on the Remote 3 and 60 Hz on the Remote Two, instead of a fixed
60 Hz. `UC_WEBVIEW_REFRESH_RATE` SHALL override the rate for a panel whose rate the UI misreports.

#### Scenario: Animation on the Remote 3
- **WHEN** a page counts its `requestAnimationFrame` callbacks while it moves a box on a Remote 3
- **THEN** it counts about 50 per second

### Requirement: Recovery after a crashed page
When the process of the page ends unexpectedly, the web view SHALL report it in the status line and
reload the page once. A second crash before a load completes SHALL NOT reload again.

#### Scenario: Web process killed
- **WHEN** the page's process is killed while a page is shown
- **THEN** the status line reports the crash and the page loads again

#### Scenario: Crash after the reload
- **WHEN** the page crashes again before its reload completed
- **THEN** the web view does not reload a second time

### Requirement: Leaving the developer page
BACK SHALL leave the developer page from any of its controls, the web view included, and HOME SHALL
close the settings. The next visit SHALL start with a new page: no history, 100 % zoom and 1×
magnification.

#### Scenario: BACK from the web view
- **WHEN** the selection is in the web view and the user presses BACK
- **THEN** the Settings menu is shown

### Requirement: Input trace without typed text
With `UC_WEBVIEW_TRACE=1` the web view SHALL log its input, keyboard, focus, zoom and load decisions
at info level for device tests, and SHALL NOT log what the user types or what the page's field holds:
no character, no key symbol or scan code of a character key, no text, length or caret position. It
SHALL log touch positions, which tell taps from drags.

#### Scenario: PIN typed with the trace on
- **WHEN** the trace is on and the user types a PIN into a page with the on-screen keyboard
- **THEN** the log shows one line per key or keyboard event and no digit of the PIN

### Requirement: Addresses in the log
Every log line of the web view SHALL show an address without user name and password, with its query
and fragment shown only as `?...` and `#...`, a `data:` address without its payload, and any address
cut at 200 characters. With `UC_WEBVIEW_LOG_FULL_URLS=1` the lines SHALL keep query and fragment for
a test device, and still leave out user name and password.

#### Scenario: Address with a token
- **WHEN** `https://user:secret@example.com/reset?token=abc#x` is loaded
- **THEN** the log shows `https://example.com/reset?...#...`

### Requirement: Dialogs of the page
With the firmware's library from 0.7.0, a page's `alert()`, `confirm()` and `prompt()` SHALL open a
dialog over the developer page. It SHALL name the site ("https://example.com says", or "This page
says" without one) and show the message as plain text, scrolling when it is long. A prompt SHALL sit
at the top with its text field and preset text, and the on-screen keyboard below.

#### Scenario: Confirm
- **WHEN** a page calls `confirm("Delete the item?")` and the user presses OK
- **THEN** a bottom sheet names the site, shows the question with OK selected, and the page gets true

#### Scenario: Prompt
- **WHEN** a page calls `prompt("Your name?", "Max")`
- **THEN** the dialog shows "Max" in its field at the top with the keyboard below, and OK returns the
  edited text

#### Scenario: Markup in a message
- **WHEN** a message contains HTML markup
- **THEN** the markup is shown as text and no image loads

### Requirement: Keys of a dialog
A dialog of the page SHALL own the keys while it is open: OK SHALL press the selected control, BACK
and HOME SHALL answer no and stay on the page, LEFT and RIGHT SHALL move between the two buttons, and
PREV, NEXT, VOLUME and CHANNEL SHALL do nothing. A tap above a bottom sheet SHALL answer no like BACK.

#### Scenario: HOME in a dialog
- **WHEN** a confirm dialog is open and the user presses HOME
- **THEN** the page gets false, the dialog closes and the developer page stays

#### Scenario: Zoom key in a dialog
- **WHEN** a dialog is open and the user presses VOLUME +
- **THEN** the page does not zoom

### Requirement: Leave-page question
When a page asks before it is left, a dialog SHALL ask "Leave this page?" with the site and the fixed
text "Changes you made may not be saved.", and SHALL open with Stay selected, so that OK stays; Leave
needs a move to it or a tap on it. The question SHALL be asked for a reload, Back, Forward and links
within the same site.

#### Scenario: OK on the question
- **WHEN** the question is shown after a tap on a link of the same site and the user presses OK
- **THEN** the page stays

#### Scenario: Link to another site
- **WHEN** a page that asks before it is left has a link to another site and the user follows it
- **THEN** the other site loads without the question, because the engine asks only for navigations
  that stay in the page's process, and a link to another site moves to a new one

### Requirement: Logins of the page
An HTTP login of the page SHALL open a full-screen login that names the site ("Sign in to
https://example.com"), quotes the server's realm as plain text, warns "Not secure: this connection is
not encrypted" for an `http:` site, and has a user name field, a password field, Cancel and Sign in.
Every login SHALL open with the password masked. Cancel SHALL continue without credentials, so that
the server's refusal page loads.

#### Scenario: Login over http
- **WHEN** a page over `http:` asks for a Basic login
- **THEN** the login shows the red "Not secure" line and the password field is masked

#### Scenario: Cancel
- **WHEN** the user cancels a login
- **THEN** the server's page for a refused login loads, and no "load failed" is shown

### Requirement: Keys of a login
OK on the user name SHALL move to the password, OK on the password SHALL sign in, BACK and HOME SHALL
cancel the login and stay on the page, and UP and DOWN SHALL walk the user name, the password and the
buttons.

#### Scenario: Signing in with the keypad
- **WHEN** the user types the user name, presses OK, types the password and presses OK
- **THEN** the credentials are sent and the login closes

### Requirement: Retry after a wrong login
After the server rejected a login answered here, the next login of the same site and realm SHALL open
with the user name filled in, an empty password field with the focus and the keyboard, and "Wrong user
name or password" below it. The user name SHALL be kept in memory only, until the retry, a load of the
page or 30 s without a retry; a login closed with Cancel, BACK or HOME SHALL start empty the next time.

#### Scenario: Wrong password
- **WHEN** the user signs in with a wrong password
- **THEN** the login opens again with the user name filled in and the password field empty and
  selected

### Requirement: Credentials are not kept or logged
The UI SHALL hold the user name and password only in the login's fields while it is open and SHALL
clear them when they are sent and when the login is cancelled or closed; the engine keeps accepted
credentials in memory until the UI restarts and writes none to disk. No log line SHALL contain the
message, preset text, answer, realm, site, user name or password of a dialog or login; the trace SHALL
show only the kind, number and outcome of each request.

#### Scenario: Journal after a login
- **WHEN** the user answered a prompt and signed in with the trace on
- **THEN** a search of the journal for the typed text, the user name, the password and the realm
  finds nothing

#### Scenario: Same site again
- **WHEN** the user signed in to a site and opens a protected page of it again in the same session
- **THEN** no login is shown until the UI restarts

### Requirement: One dialog or login at a time
The developer page SHALL show one dialog or login at a time: first the dialogs, which block the page,
then the logins in the order they came. A waiting request that the page withdraws SHALL leave the
queue, a shown one SHALL close without an answer, and a withdrawn login that was shown SHALL be
announced with "The page cancelled the sign-in".

#### Scenario: Dialog during a login
- **WHEN** a login is waiting and the page opens an alert
- **THEN** the alert is shown first, and the login after it

### Requirement: Requests without a dialog
A request that no screen shows SHALL get the engine's own answer at once: a dialog no, the leave-page
question leave, a login without credentials. With a firmware library older than 0.7.0 no dialog or
login SHALL open, and pages SHALL behave as before. When the screen itself navigates the web view while
the leave-page question waits, the web view SHALL answer it with leave; when the screen stops the load,
with stay.

#### Scenario: Older library
- **WHEN** a page calls `confirm()` with a library older than 0.7.0
- **THEN** no dialog opens and the page gets false

#### Scenario: Screen loads another address
- **WHEN** the screen loads another address while the leave-page question waits
- **THEN** the question is answered leave and the address loads

### Requirement: Content the web view cannot show
A download, a PDF document or another type the web view cannot show SHALL NOT be reported as a
failed load: the page SHALL stay, a notification SHALL say "Downloads are not supported", "PDF
documents cannot be shown here" or "This document cannot be shown here", and the status line SHALL
name the address.

#### Scenario: PDF link
- **WHEN** the user follows a link to a PDF document
- **THEN** the page stays, the notification "PDF documents cannot be shown here" appears, and no
  "load failed" is shown

### Requirement: New windows and closing
A link or script that opens a new window SHALL load it in the same web view without a message. A new
window the engine refuses, such as a `data:` address or a form posted to a new window, SHALL show "The
page tried to open a link that cannot be shown". `window.close()` SHALL go one step back in the
history, or leave the developer page with "The web page closed itself" when there is none.

#### Scenario: Link with a target
- **WHEN** the user follows a link with `target=_blank`
- **THEN** the linked page loads in the web view and PREV returns

#### Scenario: Page closes itself
- **WHEN** the first page loaded after opening the developer page calls `window.close()`
- **THEN** the notification "The web page closed itself" appears and the Settings menu is shown
