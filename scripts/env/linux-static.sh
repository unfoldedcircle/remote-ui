# Runtime environment for the statically linked Linux desktop simulator built by `make linux-static`.
# Source it in the shell that starts the app:   . scripts/env/linux-static.sh && binaries/Linux-x64-static/remote-ui
# Every value is a default: set the variable before sourcing to override it.
# Qt and its plugins are linked into the binary: no QTDIR, LD_LIBRARY_PATH or QT_PLUGIN_PATH needed.
# Build: docs/static-compile-debian-13.md. App variables: README.md, "Environment Variables".

# xcb also works on a Wayland desktop (via Xwayland). eglfs is for the device only and aborts on a desktop.
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-xcb}"

# Desktop simulator settings
export UC_MODEL="${UC_MODEL:-DEV}"
export UC_DISPLAY_WIDTH="${UC_DISPLAY_WIDTH:-480}"
export UC_DISPLAY_HEIGHT="${UC_DISPLAY_HEIGHT:-850}"
# 1 is the app default on Linux (regular display); use 0.5 on a desktop scaled to 200 %.
export UC_DISPLAY_SCALE="${UC_DISPLAY_SCALE:-1}"
# Access token of the core-simulator (https://github.com/unfoldedcircle/core-simulator)
export UC_TOKEN_PATH="${UC_TOKEN_PATH:-$HOME/projects/core-simulator/docker/ui-env/ws-token}"
