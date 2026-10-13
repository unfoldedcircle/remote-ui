One pull request, stacked on the last pull request of `lcd-readability-design-system`. Archive
this change after it is merged to `main`.

## 1. Implementation

- [x] 1.1 `src/qml/components/entities/TitleStatus.qml`: integration icon, Wi-Fi, battery and command
      spinner in one row; registered in `resources/qrc/main.qrc`
- [x] 1.2 `BaseTitle.qml` uses it; the name ends at the row and is cut off after three lines
- [x] 1.3 `BaseDetail.qml` exposes `integrationDisconnected` and no longer draws the icon itself
- [x] 1.4 `Activity.qml` uses the row instead of its own Wi-Fi and battery copy; the name and hint end
      at the row
- [x] 1.5 `CHANGELOG.md` entry

## 2. Specs

- [x] 2.1 `entity-detail-controls`: MODIFIED "Control screen title bar", ADDED "Status cluster layout"
- [x] 2.2 `activities`: MODIFIED "Activity screen header reflects failure", ADDED "Status cluster in
      the activity header"; the new layout is its own requirement instead of lengthening the modified
      ones. Verified with `openspec validate entity-title-status-row --strict`
- [x] 2.3 `adr.md` review manifest (no new ADR)

## 3. Verification

- [x] 3.1 Desktop (`UC_MODEL=DEV`, 800 px), before and after: a macro screen and the activity screen
      with the integration disconnected, Wi-Fi down, the battery shown everywhere and without it, and
      a name of 80 characters
- [ ] 3.2 Both remotes, with the device checks of `lcd-readability-design-system`
