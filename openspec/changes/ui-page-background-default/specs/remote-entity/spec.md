## MODIFIED Requirements

### Requirement: Remote page background
While a page of the remote's `user_interface` is shown, the remote's screen SHALL show the page's background, its own or the default background of the `user_interface`, behind the entity icon and name, the buttons and the page indicator, as specified in `ui-resources` ("UI page background image" and the requirements that follow it). A page without either SHALL be black. Switching pages by swipe or with DPAD_LEFT / DPAD_RIGHT SHALL slide the background with the page.

#### Scenario: Page with a background
- **WHEN** the current page of a remote has a background image
- **THEN** the image fills the remote screen behind the icon and name, the buttons and the page indicator

#### Scenario: Pages with and without a background
- **WHEN** the user swipes from a page without a background to a page with one
- **THEN** the background slides in together with the page's buttons

#### Scenario: Default background of the remote
- **WHEN** the remote's `user_interface` has a default background and one page has a background of its own
- **THEN** that page shows its own background and every other page shows the default background
