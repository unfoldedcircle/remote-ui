## MODIFIED Requirements

### Requirement: UI page background image
A page of an activity's or a remote's `user_interface` MAY carry a `background` with an image identifier `image` and the settings `scale`, `align` and `dim`. While a page is shown, its background, its own or the default one (see "Default UI page background"), SHALL fill the whole activity or remote screen behind the title, the page items and the page indicator. A page without a background, or whose image resolves to nothing, SHALL be black. The image SHALL be decoded asynchronously and cached.

#### Scenario: Page with a background image
- **WHEN** the current page's background image is `custom:tv.jpg` and the file exists
- **THEN** the image fills the screen and the title, buttons and page indicator are drawn over it

#### Scenario: Missing image file
- **WHEN** the current page's background image does not exist in the `BackgroundImage` folder
- **THEN** the screen behind the page is black

#### Scenario: Background removed
- **WHEN** a `user_interface` update sets the current page's `background` to `null`
- **THEN** the page shows the default background, or is black when there is none

#### Scenario: Failed activity
- **WHEN** an activity with a page background is in Error
- **THEN** the red title, the activity menu and the overlay of an unavailable entity are still drawn over the background

## ADDED Requirements

### Requirement: UI page background slides with its page
While the pages are dragged, flicked or switched with DPAD_LEFT / DPAD_RIGHT, each page's background SHALL move horizontally together with the page's items, in step with them, also across the wrap between the last and the first page. The title and the page indicator SHALL stay in place. A page without a background SHALL slide as black. A background whose image is not decoded yet SHALL be black until it is.

#### Scenario: Swiping to a page with another image
- **WHEN** the user drags from a page with background A halfway to the next page, which has background B
- **THEN** the left half of the screen shows the right half of A and the right half shows the left half of B, each under its own scrim, and both move with the finger

#### Scenario: Switching pages with the d-pad
- **WHEN** the user presses DPAD_RIGHT on a page with background A and the next page has no background
- **THEN** A slides out to the left together with the page's items while the black next page slides in from the right

#### Scenario: Wrap from the last to the first page
- **WHEN** the user swipes from the last page to the first page and they have different backgrounds
- **THEN** the first page's background slides in from the right together with its items

#### Scenario: Image not decoded yet
- **WHEN** the user flicks to a page whose image has not been decoded yet
- **THEN** the page slides in black and its image appears as soon as it is decoded

### Requirement: Default UI page background
An activity's or a remote's `user_interface` MAY carry a `background` with the same fields and defaults as a page background. Every page whose own `background` is absent, `null` or has an empty `image` SHALL show this default background. A page with a non-empty `image` of its own SHALL show its own background, also when that image resolves to nothing. Without a default background, such a page SHALL be black, as before.

#### Scenario: Page without a background of its own
- **WHEN** `user_interface.background` has the image `custom:room.jpg` with `dim` 30 and a page has no `background`
- **THEN** that page shows `room.jpg` with the default's scale and alignment under a 30 % scrim

#### Scenario: A page's own background wins
- **WHEN** a page has the background image `custom:tv.jpg` and the default background is `custom:room.jpg`
- **THEN** that page shows `tv.jpg` with its own settings

#### Scenario: Empty image falls back to the default
- **WHEN** a page's `background` has an empty `image` and a default background is set
- **THEN** that page shows the default background

#### Scenario: Missing own image
- **WHEN** a page's own background image does not exist in the `BackgroundImage` folder and a default background is set
- **THEN** that page is black

#### Scenario: Default background removed
- **WHEN** a `user_interface` update removes the default background
- **THEN** the pages without a background of their own are black

#### Scenario: Core without a default background
- **WHEN** the `user_interface` carries no `background`
- **THEN** every page shows its own background or is black, as before

### Requirement: Shared UI page background stays in place
Neighbouring pages whose backgrounds have the same image file, `scale` and `align` SHALL share one still background while the user swipes between them; only the items move. If their `dim` differs, the scrim SHALL change gradually with the swipe position from one page's strength to the other's. The last and the first page are neighbours too. Neighbouring pages without a background SHALL form one black area.

#### Scenario: Same image on neighbouring pages
- **WHEN** the user swipes between two neighbouring pages whose backgrounds have the same image, scale and alignment
- **THEN** the image does not move and only the items slide

#### Scenario: Only the dim differs
- **WHEN** two neighbouring pages show the same image with `dim` 20 and `dim` 60 and the user drags halfway from one to the other
- **THEN** the image stays still under a 40 % scrim

#### Scenario: Same image, other alignment
- **WHEN** two neighbouring pages show the same image, one with `align` `top` and one with `align` `bottom`
- **THEN** each background slides with its page

#### Scenario: Shared group next to another background
- **WHEN** pages 1 and 2 show image A, page 3 shows image B, and the user swipes from page 2 to page 3
- **THEN** A slides out to the left together with page 2 and B slides in with page 3

#### Scenario: Shared across the wrap
- **WHEN** the first and the last page show the same image with the same settings and the user swipes from the last to the first page
- **THEN** the image does not move

#### Scenario: One background for all pages
- **WHEN** only the default background is set and no page has a background of its own
- **THEN** the background never moves while swiping

### Requirement: UI page background update while shown
When a `user_interface` update changes the background of the shown page, the screen SHALL keep showing the old background until the new image is decoded and then show the new one, without a black frame in between. An update that leaves the shown background unchanged SHALL NOT change it visibly.

#### Scenario: Background changed in the web-configurator
- **WHEN** a `user_interface` update changes the image of the current page's background
- **THEN** the old image stays until the new one is decoded, then the new one is shown without leaving the page

#### Scenario: Default background changed
- **WHEN** a `user_interface` update changes the default background while a page without a background of its own is shown
- **THEN** the page shows the new default background the same way

#### Scenario: Unrelated update
- **WHEN** a `user_interface` update changes only a button of the current page
- **THEN** the background does not change visibly

## REMOVED Requirements

### Requirement: UI page background transitions
**Reason**: The background no longer cross-fades on a page change; it slides with its page.
**Migration**: Replaced by "UI page background slides with its page", "Shared UI page background stays in place" and "UI page background update while shown".
