# Remote UI Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---
## v0.68.5 - 2026-01-15
### Fixed
- Entity state check before starting/stopping activities

---
## v0.68.4 - 2026-01-14
### Fixed
- Button control not working when entity opened from an activity
- Activity page indicator visible when activity in header is disabled

---
## v0.68.3 - 2026-01-13
### Fixed
- Dropdown menu button control. Mainly present in activity included entities screen letting button presses through.

---
## v0.68.2 - 2026-01-12
### Fixed
- Long press timer key tracking

---
## v0.68.1 - 2026-01-11
### Fixed
- Media image not shown on page entity

---
## v0.68.0 - 2026-01-09
### Fixed
- Button navigation sproadically stops working
- Voice assistant listening animation still showed after error
- Activity start screen with 0 included entities
- Activity error handling for sequences
- Entity name missing when starting activity for first time
- Text cut off on activity loading screen
- Ignore button presses for unavailable entities

---
## v0.67.0 - 2025-12-24
### Fixed
- Touch slider warning if entity is unavailable
- Sensor widget shows wrong values in activity UI

---
## v0.66.0 - 2025-12-19
### Fixed
- Same sensor value shown for all sensors
- Customer sensor label shows "Custom"
- Voice UI lets button presses through
- Processing showed after voice assistant finished event
- Activity sequence timeout handling
- Activity's entities readyness check
- Invalid media player state
- Media image not shown on main page entities
- Touch slider commands even when entity is unavailable

### Changed
- Improved voice assistant error handling
- Improved activity turn on/off after resume from system sleep

---
## v0.65.10 - 2025-12-10
### Fixed
- Charging screen shown after reboot

---
## v0.65.2 - 2025-12-05
### Added
- Voice Assistant support
- Command retry after wakeup. Wakeup window is configurable in Power Saving settings.

### Fixed
- Media image download timeout handling

### Changed
- Disable certificate validation for media image download

---
## v0.64.4 - 2025-11-27
### Fixed
- Media image not loaded sproadically
- Sensor value not shown within activity

---
## v0.64.3 - 2025-11-23
### Changed
- Display brightness minimum value to 5%


---
## v0.64.1 - 2025-11-21
### Fixed
- Only load media image when it has changed

---
## v0.64.0 - 2025-11-18
### Added
- Touch slider configuration support

---
## v0.63.0 - 2025-11-06
### Added
- Sensor widget support for activities
- Notify before starting an activity if an integration is not ready
- Notify with option to try again if command fails due to device not being ready

### Changed
- Charging screen shows up when a power supply is detected with additional information if the device is charging or just being supplied with power
- Show IP address instead of hostname by default for the Web Configurator

### Fixed
- Sensor UI screens
- Popup menu button handling
- Visilbity of software update icon in the status bar
- Failed marco sequences not shown
- Wifi network list empty during dock setup
- Popup menu trims text for long text items

---
## v0.62.2 - 2025-09-26
### Fixed
- Popup menu button handling

---
## v0.62.0 - 2025-09-23
### Changed
- Reload entity data when entering UI screen
- Update method for loading button mapping

---
## v0.61.0 - 2025-09-22
### Changed
- Starting an activity from another activity will open the new activity's UI

### Fixed
- Rendering of icons
- Show loading icon next to WiFi networks, when connecting

---
## v0.60.1 - 2025-09-19
### Added
- Binary sensor support

### Fixed
- Known WiFi network did not connect when selected

### Changed
- WiFi settings menu

---
## v0.59.0 - 2025-09-12
### Fixed
- Repeat command handling, do not wait for ack

### Changed
- Repeat count increased to 4

---
## v0.58.3 - 2025-08-27
### Fixed
- QR code in pull-down menu and during onboarding
- Popup menu closed when home button released when it has opened

---
## v0.58.2 - 2025-08-26
### Fixed
- QR code in pull-down menu and during onboarding
- Popup menu closed when home button released when it has opened

---
## v0.58.0 - 2025-08-25
### Fixed
- QR code in pull-down menu and during onboarding
- Popup menu closed when home button released when it has opened

---
## v0.57.0 - 2025-08-18
### Fixed
- Language text logic
- High power consumption when display is off

### Changed
- Renamed media image fill option

---
## v0.56.4 - 2025-08-05
### Fixed
- High CPU consumption in low power mode

---
## v0.56.3 - 2025-08-03
### Fixed
- Wifi scan interval slider range

---
## v0.56.2 - 2025-08-02
### Fixed
- High CPU consumption while loading animation is running

---
## v0.56.0 - 2025-07-24
### Added
- WiFi band selection
- WiFi scan interval config option

---
## v0.55.1 - 2025-07-04
### Fixed
- Incorrect dock image shown
- Dock discovery help text

---
## v0.54.10 - 2025-06-06
### Fixed
- Bug in repeat logic

---
## v0.54.9 - 2025-05-27
### Fixed
- Wifi icon size in known networks
- Transparent media image when no media text is shown
- Media player screen shuffle, repeat and app icons cut off
- Activity bar height jumps when image changes
- Media image sometimes not shown
- Touch slider not working with certain device classes

---
## v0.54.5 - 2025-05-23
### Fixed
- Turn off menu only shows entities with on/off features available

## v0.54.4 - 2025-05-19
### Changed
- Media type is displayed as string

### Fixed
- Record, Stop and Menu buttons not working on Remote 3
- Icon shown under transparent media image

## v0.54.2 - 2025-05-12
### Fixed
- Icon shown under transparent media image

## v0.53.2 - 2025-04-06
### Added
- Option to fill available space for media player widget. Can be turned on in Settings / User interface.

### Fixed
- Activity list image and icon sizes
- Media player widget shrinking

## v0.50.2 - 2025-04-03
### Fixed
- Missing media player icon map

## v0.50.0 - 2025-03-31
### Added
- Support for touch slider
- Access profiles, Web Configurator and settings by pulling down the page

### Changed
- Activity bar moved to the page header with option to turn it off in Settings / User Interface

### Fixed
- Missing icons during dock discovery
- Wrong remote name for Remote 3 during onboarding

## v0.49.0 - 2024-02-11
### Added
- Option to show media widget as horizontal

## v0.48.0 - 2024-01-17
### Fixed
- DPAD middle button behaviour on pages
- Sizing of media player widget on activity UI pages. Very small media widget won't show progress bar and media information.
