## ADDED Requirements

### Requirement: The web engine is a firmware library loaded at runtime
The UI SHALL NOT link a web engine. It SHALL show web pages only through the firmware's web view
library (`libucwebview.so.1`), loaded at runtime through the library's versioned plain C interface,
and SHALL use it only when the library's API version equals the one the UI was built for (ADR 0021).
Every UI build SHALL run without the library, and a build SHALL be possible that leaves the web view
out.

#### Scenario: Firmware without the library
- **WHEN** a UI build with the web view runs on a firmware that does not ship the library
- **THEN** it starts and works as before, and the web view is unavailable

#### Scenario: Build without the web view
- **WHEN** the app is built with `CONFIG+=no-webview`
- **THEN** the binary contains none of the web view's C++ code, needs no private Qt module for it, and
  the settings never list the "Web view" entry

### Requirement: The web view costs nothing before it is used
The UI SHALL NOT load the web view library on the start-up path, and no web content process SHALL run
before the first web view is created. A web view that is hidden, or whose window is hidden because the
display is off, SHALL render nothing until it is shown again.

#### Scenario: Start-up with the library present
- **WHEN** the UI starts on a firmware that ships the library
- **THEN** the library is loaded only when the UI first asks whether a web view is available, which is
  when the Settings menu is first built, and the start-up time is unchanged

#### Scenario: Display off with a page open
- **WHEN** the display turns off while the developer page shows a page
- **THEN** the page renders no frame until the display is on again
