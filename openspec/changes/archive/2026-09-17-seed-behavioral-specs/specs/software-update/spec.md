## ADDED Requirements

### Requirement: Update check on connection
On every successful authentication the UI SHALL send a non-forced `check_system_update` (force=false) and a `version` request. Failure of this check SHALL show the actionable warning "Update check failed" / "There was an error while checking for new updates. Please try again later.".

#### Scenario: Reconnect
- **WHEN** the UI reconnects to the core
- **THEN** update availability and version information are refreshed

### Requirement: Update check from the settings page
Opening the Software update page SHALL run a non-forced check, and while the page is open a silent non-forced check SHALL repeat every 3000 ms (silent: no notification on failure). "Check for update" SHALL run a forced check (`force=true`) and report failure with the "Update check failed" notification. "Check for update" is hidden while a download is running.

#### Scenario: Manual check fails
- **WHEN** the forced check returns an error or times out
- **THEN** the "Update check failed" notification is shown

#### Scenario: Background refresh
- **WHEN** the page is open and the core reports download progress via the check response
- **THEN** the page reflects it within 3 s without user action

### Requirement: Update information shown
From a check response the UI SHALL take `installed_version` as "Current version" and the first entry of `available` as the offered update: "New version", download state (Pending / Downloading / Downloaded / Error) and release notes composed as `<title>` + blank line + description in the UI language. When the download state is Error the update is still flagged available but version and release notes are not refreshed from that entry. With no entries the page reads "Your software is up to date"; otherwise "New software version is available". `update_in_progress` true in the response SHALL mark an update as in progress.

#### Scenario: Update available
- **WHEN** the check returns one available update in state Downloaded
- **THEN** the page shows current and new version, "Downloaded" and an "Install" button

#### Scenario: Download error
- **WHEN** the first available update reports download state Error
- **THEN** the state row reads "Error" and the previously shown version and release notes remain

### Requirement: Update indicators outside the page
While an update is available the status bar SHALL show a yellow "cloud with arrow" icon, and the settings menu and the profile menu SHALL show a badge on the "Software update" entry.

#### Scenario: Badge
- **WHEN** an update becomes available
- **THEN** the icon and badges appear without opening the settings

### Requirement: Download progress
While the download state is Downloading the page SHALL show a progress bar and "N%" from the download percentage of the progress events, and the install button SHALL be disabled (30 % opacity). A `download_percent` of 100 or more SHALL switch the state to Downloaded.

#### Scenario: Download running
- **WHEN** progress events with state DOWNLOAD arrive
- **THEN** the bar and percentage follow each event

### Requirement: Starting an update
The install button reads "Download" until the update is Downloaded, then "Install". Pressing it SHALL require a battery level above 50 %; otherwise the actionable warning "Low battery" / "Minimum 50% battery charge is required to install software updates" is shown and nothing is sent. Otherwise the UI SHALL send `update_system` with the offered update id followed by a non-forced check; a rejected `update_system` shows "Update error" / "Couldn't start the software update. Please try again later.".

#### Scenario: Battery at 50 %
- **WHEN** the battery level is exactly 50 %
- **THEN** the low-battery warning is shown (strictly greater than 50 required)

#### Scenario: Start accepted
- **WHEN** the core acknowledges `update_system`
- **THEN** the UI waits for `software_update` events; no screen changes until START arrives

### Requirement: Update progress screen
A `software_update` event of type START SHALL open a non-dismissable "Update in progress" screen that blocks all input, with a progress bar, "Installing step <current>/<total> <percent>%" and "Do not turn off the remote during the installation process!". PROGRESS events update step count, step index and percentage. Progress state SUCCESS, or a STOP event whose state is not FAILURE, SHALL show "Update success" / "Software update was successful. The remote will reboot now." FAILURE (in a PROGRESS or STOP event) SHALL show "Update failed" / "There was an error during installing the update." with a "Back" button, unblock input, and reset steps to 0/0. The failure screen SHALL be closable with the Back button (reachable with the d-pad), BACK or HOME.

#### Scenario: Successful install
- **WHEN** the core sends progress state SUCCESS
- **THEN** the success screen is shown and stays until the core reboots the device

#### Scenario: Failed install
- **WHEN** a STOP event with state FAILURE arrives
- **THEN** the failure screen is shown; pressing Back, BACK or HOME closes it and returns to the settings

### Requirement: Power off blocked during an update
A long press of the power button SHALL NOT open the power-off screen while an update is in progress.

#### Scenario: Long press during install
- **WHEN** the user long-presses POWER while the progress screen is shown
- **THEN** nothing happens

### Requirement: Version information
The About page SHALL show "UI version" (the UI build version), "Core version" and "System version" (the installed version from the last update check). The API version reported by the core is kept for display and diagnostics. Version fields are refreshed from the `version` response at every connection.

#### Scenario: About page
- **WHEN** the About page opens after a connection
- **THEN** UI, core and system versions are populated (or "N/A" for the system version before the first check)

### Requirement: Release notes
The "Release notes" row (shown only while an update is available, and the first focused row in that case) SHALL open a scrollable page titled "Release Notes" with the composed notes; DPAD_UP / DPAD_DOWN scroll it, BACK closes it and the focus returns to the row it was opened from. The notes SHALL be recomposed in the new language whenever the UI language changes, using the description entry for the language (fallbacks: exact code, base language, default country variant, first other variant of the language, English).

#### Scenario: Language switched
- **WHEN** the UI language changes while an update is offered
- **THEN** the release notes text switches to that language without a new check

### Requirement: Update settings
The page SHALL expose the "Check for updates" and "Auto update" toggles, the OTA window text and the "Beta updates" row as specified in the device-configuration capability.

#### Scenario: Testing channel
- **WHEN** the core reports channel TESTING
- **THEN** "Beta updates — Enabled" is shown on the page
