## ADDED Requirements

### Requirement: Activity page background
While a page of the activity's `user_interface` that has a `background` is the current page, the activity screen SHALL show that background behind the title, the page items and the page indicator, as specified in `ui-resources` ("UI page background image" and the requirements that follow it). A page without a background SHALL keep the black background. Switching pages by swipe or with DPAD_LEFT / DPAD_RIGHT SHALL change the background with the page.

#### Scenario: Page with a background
- **WHEN** the current page of an activity has a background image
- **THEN** the image fills the activity screen behind the title, the items and the page indicator

#### Scenario: Switching pages with the d-pad
- **WHEN** the user presses DPAD_RIGHT on a page with background A and the next page has no background
- **THEN** the next page is shown and the background fades from A to black
