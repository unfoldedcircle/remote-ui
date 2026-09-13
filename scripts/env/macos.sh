# Runtime environment for the macOS desktop simulator ("Remote UI.app", built with Qt Creator or a static Qt kit,
# see docs/static-compile-macos.md). Source it in the shell that starts the app, e.g.
#   . scripts/env/macos.sh && "binaries/macOS-x64/Remote UI.app/Contents/MacOS/Remote UI"
# or enter the same variables in the Qt Creator run settings.
# Every value is a default: set the variable before sourcing to override it.
# App variables: README.md, "Environment Variables".

# Desktop simulator settings
export UC_MODEL="${UC_MODEL:-DEV}"
export UC_DISPLAY_WIDTH="${UC_DISPLAY_WIDTH:-480}"
export UC_DISPLAY_HEIGHT="${UC_DISPLAY_HEIGHT:-850}"
# 0.5 (the app default) is right for a 2x Retina display; use 1 on a non-Retina display.
export UC_DISPLAY_SCALE="${UC_DISPLAY_SCALE:-0.5}"
# Access token of the core-simulator (https://github.com/unfoldedcircle/core-simulator)
export UC_TOKEN_PATH="${UC_TOKEN_PATH:-$HOME/projects/core-simulator/docker/ui-env/ws-token}"
