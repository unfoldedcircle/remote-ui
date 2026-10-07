## MODIFIED Requirements

### Requirement: Actionable notification presentation
An actionable notification SHALL cover the whole screen with a gradient from transparent at the top to black from 60 % of the height downwards, and SHALL show, from the bottom up: the action label (bold 26 px, right half) and "Cancel" (bold 26 px, left half) 30 px above the bottom edge, both only when an action label is given; the message (24 px, 70 % opacity) 40 px above them; the title (40 px) 20 px above the message; and a 140 px icon above the title, which SHALL be a warning triangle when no icon is given. A 4 px bar along the bottom edge SHALL pulse (1000 ms fade out, 1000 ms fade in, 2000 ms pause, repeated). Warning notifications SHALL draw the icon and the bar in red; neutral ones draw the icon in off-white and the bar in the highlight colour. The notification SHALL fade in over 300 ms and out over 200 ms, and SHALL stay until the user dismisses it; it never closes on a timer.

#### Scenario: Warning with an action
- **WHEN** "Delete all networks" is requested in the WiFi settings
- **THEN** a notification with a red warning icon, "Delete all networks", "Are you sure you want to delete all WiFi networks?", "Cancel" on the left and "Delete all" on the right is shown with a pulsing red bar

#### Scenario: Neutral notification without an action
- **WHEN** "Select entities" / "Please select the entities to add in the list." is shown
- **THEN** no action label and no "Cancel" are shown and the notification stays until dismissed
