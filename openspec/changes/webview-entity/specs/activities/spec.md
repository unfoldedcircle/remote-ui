## ADDED Requirements

### Requirement: Web view widget on activity pages
An activity page SHALL render a `webview` item as the page of its web view entity in the grid: loaded while that page
is the current one and the activity screen is open, unloaded otherwise and while a fullscreen web view is open. The
item's `zoom` SHALL replace the entity's zoom. Only the first `webview` item of an activity SHALL be rendered. Without
the web view, for an unavailable entity or without a start page it SHALL show an empty placeholder.

#### Scenario: Interactive widget
- **WHEN** the user swipes on an interactive web view widget
- **THEN** the page scrolls and the activity page does not change

#### Scenario: Widget that is not interactive
- **WHEN** the user taps a web view widget with `interactive` false
- **THEN** the web view opens fullscreen on top of the activity screen

#### Scenario: Keys
- **WHEN** the user touched an interactive web view widget and then presses DPAD_RIGHT
- **THEN** the activity shows its next page, as the button mapping says, and the web page gets no key

#### Scenario: Leaving the page
- **WHEN** the user swipes to another page of the activity
- **THEN** the web page of the widget is unloaded and loaded again when its page is shown again

### Requirement: Load indicator of a web view widget
A web view widget SHALL show the load indicator of its web view at its size: the percentage only where it fits, the
failure without the OK hint and without buttons, its reason line only where it fits. A tap on the failure SHALL load the
page again when the widget is interactive.

#### Scenario: Failed page in a widget
- **WHEN** the page of an interactive 3 x 3 web view widget cannot be loaded
- **THEN** the widget shows "The page could not be loaded" without "Press OK to try again", and a tap loads the page
  again

#### Scenario: Failed page in a widget that is not interactive
- **WHEN** the page of a web view widget with `interactive` false cannot be loaded and the user taps the widget
- **THEN** the web view opens fullscreen, as for every tap on that widget
