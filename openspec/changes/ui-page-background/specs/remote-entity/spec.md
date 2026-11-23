## ADDED Requirements

### Requirement: Remote page background
While a page of the remote's `user_interface` that has a `background` is the current page, the remote's screen SHALL show that background behind the entity icon and name, the buttons and the page indicator, as specified in `ui-resources` ("UI page background image" and the requirements that follow it). A page without a background SHALL keep the black background. Switching pages by swipe or with DPAD_LEFT / DPAD_RIGHT SHALL change the background with the page.

#### Scenario: Page with a background
- **WHEN** the current page of a remote has a background image
- **THEN** the image fills the remote screen behind the icon and name, the buttons and the page indicator

#### Scenario: Pages with and without a background
- **WHEN** the user swipes from a page without a background to a page with one
- **THEN** the background fades in from black
