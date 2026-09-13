# Qt installation on Ubuntu 22.04

Part of the [installation instructions](install.md). This is the original guide for a minimal Ubuntu 22.04 desktop
installation. It should also work for other Debian-based distributions with small adaptations; Debian 13 has its own
guide in [install-debian-13.md](install-debian-13.md).

## Install aqt installer

The remote-ui simulator is a Qt application and requires the Qt framework library. Installing Qt with the system's
package manager usually doesn't work. We suggest using [Another Qt installer (aqt)](https://github.com/miurahr/aqtinstall)
to install the required Qt runtime.

```bash
sudo apt install python3-pip
pip install aqtinstall
```

Add the local binary directory `~/.local/bin` to the search path (if not yet included):

1. Edit `~/.bashrc`
2. At the end of the file, add: `PATH="$HOME/.local/bin:$PATH"`

## Install Qt Runtime

Requirements:
- Qt version: 5.15.2
- Qt features: multimedia, qml, quick, quickcontrols2, virtualkeyboard, websockets

```bash
mkdir ~/Qt
aqt install-qt --outputdir ~/Qt linux desktop 5.15.2 gcc_64 -m qtvirtualkeyboard
```

Notes:
- Most Qt modules are already included in the default installation and therefore not included in the module parameter.
- See [aqtinstall docs](https://aqtinstall.readthedocs.io/en/latest/getting_started.html) for further information.
- Ubuntu 22.04 ships GCC 11, which needs the `<limits>` header patch described in
  [install-debian-13.md, step 3](install-debian-13.md#3-header-patch-for-gcc--11).

## Configure Environment

Some environment variables need to be set for the installed Qt runtime. `aqtinstall` will not set them automatically, to
not interfere with an existing installation.

```bash
export QT_VERSION=5.15.2
export QTDIR="$HOME/Qt/$QT_VERSION/gcc_64"
export PATH="$QTDIR/bin:$PATH"
export LD_LIBRARY_PATH="$QTDIR/lib:$LD_LIBRARY_PATH"
export QT_PLUGIN_PATH="$QTDIR/plugins"
export QT_QPA_PLATFORM=wayland
```

Attention if not using Ubuntu 22.04: `QT_QPA_PLATFORM` must be set to the correct graphical environment.  
E.g. for Lubuntu it's already set to `lxqt`. See Qt docs for more information.

If you are using a dedicated VM, you can add the required environment variables to the end of `~/.profile`. Otherwise,
it's better to create a start script for launching the UI app, or use `scripts/env/linux.sh` from the repository:
it sets the same variables plus the app settings from `README.md`, all overridable. Its platform default is `xcb`;
for the Wayland plugin used here, export the platform before sourcing:

```bash
export QT_QPA_PLATFORM=wayland
. scripts/env/linux.sh && binaries/Linux-x64/remote-ui
```

## Build and run

```bash
make linux        # -> binaries/Linux-x64/remote-ui
make test         # unit tests
make run-linux    # start the UI app (Remote-Core Simulator must be running, see install.md)
```

`make` without a target lists all targets. Alternatively open `remote-ui.pro` in Qt Creator with a kit based on
`~/Qt/5.15.2/gcc_64/bin/qmake`.
