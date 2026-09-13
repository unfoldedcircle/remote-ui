# Runtime environment for the dynamically linked Linux desktop simulator built by `make linux`.
# Source it in the shell that starts the app:   . scripts/env/linux.sh && binaries/Linux-x64/remote-ui
# Every value is a default: set the variable before sourcing to override it.
# Qt setup: docs/install.md (per-target guides linked there). App variables: README.md, "Environment Variables".

# Qt 5.15.2 installed with aqtinstall
export QT_VERSION="${QT_VERSION:-5.15.2}"
export QTDIR="${QTDIR:-$HOME/Qt/$QT_VERSION/gcc_64}"
export PATH="$QTDIR/bin:$PATH"
export LD_LIBRARY_PATH="$QTDIR/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="$QTDIR/plugins"
# xcb also works on a Wayland desktop (via Xwayland). eglfs is for the device only and aborts on a desktop.
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-xcb}"

# Desktop simulator settings
export UC_MODEL="${UC_MODEL:-DEV}"
export UC_DISPLAY_WIDTH="${UC_DISPLAY_WIDTH:-480}"
export UC_DISPLAY_HEIGHT="${UC_DISPLAY_HEIGHT:-850}"
# 1 on a regular display. The app default 0.5 is meant for 2x (Retina / 200 % scaled) displays.
export UC_DISPLAY_SCALE="${UC_DISPLAY_SCALE:-1}"
# Access token of the core-simulator (https://github.com/unfoldedcircle/core-simulator)
export UC_TOKEN_PATH="${UC_TOKEN_PATH:-$HOME/projects/core-simulator/docker/ui-env/ws-token}"
