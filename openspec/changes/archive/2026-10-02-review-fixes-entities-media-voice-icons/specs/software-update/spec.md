## MODIFIED Requirements

### Requirement: Update information shown
From a check response the UI SHALL take `installed_version` as "Current version" and the first entry of `available` as the offered update: "New version", download state (Pending / Downloading / Downloaded / Error) and release notes composed as `<title>` + blank line + description in the UI language. When the download state is Error the update is still flagged available but version and release notes are not refreshed from that entry. With no entries the page reads "Your software is up to date"; otherwise "New software version is available". The UI SHALL take `update_in_progress` from every check response both ways: true SHALL mark an installation as in progress, and false SHALL clear that mark, so an installation whose end the UI missed — for example while it was reconnecting to the core — does not stay marked as in progress.

#### Scenario: Update available
- **WHEN** the check returns one available update in state Downloaded
- **THEN** the page shows current and new version, "Downloaded" and an "Install" button

#### Scenario: Download error
- **WHEN** the first available update reports download state Error
- **THEN** the state row reads "Error" and the previously shown version and release notes remain

#### Scenario: Installation end missed while disconnected
- **WHEN** the UI marked an installation as in progress, lost the core connection before the installation ended, and the check after the reconnection reports `update_in_progress` false
- **THEN** the installation is no longer marked as in progress

### Requirement: Download progress
While the download state is Downloading the page SHALL show a progress bar and "N%" from the download percentage of the progress events, and the install button SHALL be disabled (30 % opacity). A `download_percent` of 100 or more SHALL switch the state to Downloaded. A download runs without a START event and reports PROGRESS events with state DOWNLOAD; a PROGRESS event with state FAILURE that arrives while no installation is in progress SHALL be taken as a failed download: the download state SHALL become Error, so the page reads "Error" and the button is enabled again, and the installation screen SHALL NOT be shown and SHALL NOT take the keys.

#### Scenario: Download running
- **WHEN** progress events with state DOWNLOAD arrive
- **THEN** the bar and percentage follow each event

#### Scenario: Download fails
- **WHEN** a download is running and a PROGRESS event with state FAILURE arrives while no installation is in progress
- **THEN** the page shows the download state "Error" with the "Download" button enabled, no failure screen is shown, and the keys keep working on the page

### Requirement: Update progress screen
The UI SHALL rely on the update protocol of the core: an installation starts with a `software_update` event of type START, reports PROGRESS events (states START, RUN, PROGRESS, SUCCESS, DONE) and the remote then reboots, or it ends with a PROGRESS event with state FAILURE; a STOP event is only sent when the update client cannot be started, and then always with state FAILURE. A START event SHALL open a non-dismissable "Update in progress" screen that blocks all input, with a progress bar, "Installing step <current>/<total> <percent>%" and "Do not turn off the remote during the installation process!". PROGRESS events update step count, step index and percentage. Progress state SUCCESS SHALL show "Update success" / "Software update was successful. The remote will reboot now." FAILURE in a PROGRESS or STOP event while the installation screen is shown SHALL show "Update failed" / "There was an error during installing the update." with a "Back" button, unblock input, and reset steps to 0/0. A FAILURE while the installation screen is not shown SHALL NOT show the failure screen and SHALL NOT take the keys (a failed download is shown on the page, see "Download progress"). The failure screen SHALL be closable with the Back button (reachable with the d-pad), BACK or HOME.

#### Scenario: Successful install
- **WHEN** the core sends progress state SUCCESS
- **THEN** the success screen is shown and stays until the core reboots the device

#### Scenario: Failed install
- **WHEN** a STOP event with state FAILURE arrives while the installation screen is shown
- **THEN** the failure screen is shown; pressing Back, BACK or HOME closes it and returns to the settings

#### Scenario: Installation fails while it runs
- **WHEN** the installation screen is shown after START and a PROGRESS event with state FAILURE arrives
- **THEN** the failure screen is shown, the input is unblocked and the steps are reset to 0/0

#### Scenario: Failure without an installation screen
- **WHEN** a FAILURE arrives while the installation screen is not shown
- **THEN** no failure screen appears and the keys stay with the screen the user is on

### Requirement: Power off blocked during an update
A long press of the power button SHALL NOT open the power-off screen while an update is in progress. An update SHALL count as in progress from its START event until its FAILURE, and for as long as the core reports `update_in_progress` true in a check response; a check response with `update_in_progress` false SHALL end it.

#### Scenario: Long press during install
- **WHEN** the user long-presses POWER while the progress screen is shown
- **THEN** nothing happens

#### Scenario: Power off after a missed installation end
- **WHEN** the end of an installation was missed while the remote reconnected to the core and the next check reports `update_in_progress` false
- **THEN** a long press of POWER opens the power-off screen again
