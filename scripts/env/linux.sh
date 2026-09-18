# Runtime environment for the dynamically linked Linux desktop simulator built by `make linux`.
# Source it in the shell that starts the app:   . scripts/env/linux.sh && binaries/Linux-x64/remote-ui
# Every value is a default: set the variable before sourcing to override it.
# Qt setup: docs/install.md (per-target guides linked there). App variables: README.md, "Environment Variables".

# Qt: the newest ~/Qt/5.x.y/gcc_64 unless QT_VERSION or QTDIR is set (`. scripts/env/qt-version.sh` does that)
export QTDIR="${QTDIR:-${QT_VERSION:+$HOME/Qt/$QT_VERSION/gcc_64}}"
export QTDIR="${QTDIR:-$(ls -d "$HOME"/Qt/5.*/gcc_64 2>/dev/null | sort -V | tail -n 1)}"
export QT_VERSION="${QT_VERSION:-$("$QTDIR/bin/qmake" -query QT_VERSION 2>/dev/null)}"
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
