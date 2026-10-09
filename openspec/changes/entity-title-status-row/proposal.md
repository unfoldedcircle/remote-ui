## Why

On an entity control screen the red link-slash icon of a disconnected integration was drawn on top of
the Wi-Fi and battery icons of the title: the screen placed it left of the close icon, while the title
placed its status icons at the same spot. With the remote off Wi-Fi, or "Show battery indicator
everywhere" on, the icons covered each other. The activity screen had its own copy of the Wi-Fi and
battery icons, both anchored to the same point, so these two could cover each other as well.

## What Changes

- One status row for the title of every entity control screen: the integration icon, the Wi-Fi icon,
  the battery indicator and the command spinner side by side, at the right of the title, clear of the
  close icon.
- The activity screen header uses the same row instead of its own copy.
- The entity name ends where the row begins and is cut off with an ellipsis after three lines, so a
  long name runs neither under the icons nor out of the 80 px bar.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `entity-detail-controls`: "Control screen title bar" places the integration icon in the status
  cluster; the new "Status cluster layout" keeps the icons side by side and bounds the name.
- `activities`: the new "Status cluster in the activity header" shows the integration icon with the
  Wi-Fi and battery icons in one row; "Activity screen header reflects failure" no longer lists them.

## Impact

- **Hardware models:** Remote Two and Remote 3, and the desktop simulator.
- **remote-core dependency:** none.
- **Third-party code:** none added.
- **Code:** `src/qml/components/entities/TitleStatus.qml` (new, registered in
  `resources/qrc/main.qrc`), `BaseTitle.qml`, `BaseDetail.qml`,
  `activity/deviceclass/Activity.qml`, `CHANGELOG.md`.
- **Origin:** found while testing the stacked pull requests of `lcd-readability-design-system` on the
  desktop simulator; the overlap predates them (reconnection notification, battery indicator
  everywhere, command spinner).
