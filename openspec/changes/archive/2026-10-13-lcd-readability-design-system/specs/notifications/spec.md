## MODIFIED Requirements

### Requirement: Actionable notification presentation
An actionable notification SHALL cover the whole screen with a gradient from transparent at the top to black from 60 % of the height downwards, and SHALL show, from the bottom up: the action as a primary button on the right and "Cancel" as a secondary button on the left, side by side, 80 px high and 30 px above the bottom edge, both only when an action label is given; the message in the prose role (26 px, line height 1.3) and the primary text colour 40 px above them; the title (40 px) 20 px above the message; and a 140 px icon above the title, which SHALL be a warning triangle when no icon is given. A 4 px bar along the bottom edge SHALL pulse (1000 ms fade out, 1000 ms fade in, 2000 ms pause, repeated). Warning notifications SHALL draw the icon and the bar in red; neutral ones draw the icon in off-white and the bar in the primary text colour. The notification SHALL fade in over 300 ms and out over 200 ms, and SHALL stay until the user dismisses it; it never closes on a timer.

#### Scenario: Warning with an action
- **WHEN** "Delete all networks" is requested in the WiFi settings
- **THEN** a notification with a red warning icon, "Delete all networks", "Are you sure you want to delete all WiFi networks?", "Cancel" as a secondary button on the left and "Delete all" as a primary button on the right is shown with a pulsing red bar

#### Scenario: Neutral notification without an action
- **WHEN** "Select entities" / "Please select the entities to add in the list." is shown
- **THEN** no buttons are shown, the bar pulses in the primary text colour, and the notification stays until dismissed
