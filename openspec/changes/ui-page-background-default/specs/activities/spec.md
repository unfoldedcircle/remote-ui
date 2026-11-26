## MODIFIED Requirements

### Requirement: Activity page background
While a page of the activity's `user_interface` is shown, the activity screen SHALL show the page's background, its own or the default background of the `user_interface`, behind the title, the page items and the page indicator, as specified in `ui-resources` ("UI page background image" and the requirements that follow it). A page without either SHALL be black. Switching pages by swipe or with DPAD_LEFT / DPAD_RIGHT SHALL slide the background with the page.

#### Scenario: Page with a background
- **WHEN** the current page of an activity has a background image
- **THEN** the image fills the activity screen behind the title, the items and the page indicator

#### Scenario: Switching pages with the d-pad
- **WHEN** the user presses DPAD_RIGHT on a page with background A and the next page has no background
- **THEN** the next page slides in black from the right while A slides out to the left with the previous page

#### Scenario: Default background of the activity
- **WHEN** the activity's `user_interface` has a default background and none of its pages has a background of its own
- **THEN** every page shows the default background, which stays in place while swiping
