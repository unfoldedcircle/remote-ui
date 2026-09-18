# Installation instructions

How to set up a development environment for the remote-ui desktop simulator: Qt 5.15, the fonts, a container
runtime for the [Remote-Core Simulator](https://github.com/unfoldedcircle/core-simulator) and optional integrations.

We highly recommend using a dedicated virtual machine, especially if you are not yet familiar with setting up a Qt
runtime environment.

## Qt

The remote-ui app is a Qt 5.15 application: 5.15.2 is the version of the GitHub workflow (the last one with official
binary packages), 5.15.19 is the final Qt 5.15 release and the recommended one for development. The Remote Two/3
devices run a 5.15.8 built by the [cross-compile toolchain](cross-compile.md). Installing Qt with the system's package
manager usually doesn't work (wrong version); the guides below build Qt 5.15.19 from source or install the official
5.15.2 binaries with [aqtinstall](https://github.com/miurahr/aqtinstall). Pick the guide for your system:

| Target                    | Guide                                                    | Notes                                                        |
|---------------------------|----------------------------------------------------------|--------------------------------------------------------------|
| Debian 13 "trixie"        | [install-debian-13.md](install-debian-13.md)             | 5.15.19 from source (recommended) or 5.15.2 with aqtinstall, both side by side; verified on a fresh VM |
| Ubuntu 22.04              | [install-ubuntu-22.04.md](install-ubuntu-22.04.md)       | the original guide, aqtinstall                               |
| macOS                     | [static-compile-macos.md](static-compile-macos.md)       | Qt online installer and Qt Creator, static kit               |
| Static desktop build      | [static-compile.md](static-compile.md)                   | self-contained binary without Qt libraries, macOS and Debian 13 |
| Remote Two/3 device       | [cross-compile.md](cross-compile.md)                     | Docker toolchain, `make ucr2`                                |

Adding a target: copy the closest guide, name it `install-<distribution>-<version>.md`, and link it in this table.

After Qt is installed, build with `make linux` (or `make linux-static`) and start the UI app with `make run-linux`;
`make` without a target lists everything. Several Qt versions can be installed side by side in `~/Qt/<version>/`:
`. scripts/env/qt-version.sh [version]` selects one for the current shell and the Makefile defaults to the newest
one (`make linux QT_VERSION=5.15.2` overrides). Qt Creator users open `remote-ui.pro` with a kit for the installed Qt.
See the [README](../README.md) for the environment variables the app reads.

## Fonts

The remote-ui uses the following Google fonts families:

- [Poppins](https://fonts.google.com/specimen/Poppins)
- [Space Mono](https://fonts.google.com/specimen/Space+Mono)

These fonts are licensed under the [Open Font License](https://scripts.sil.org/cms/scripts/page.php?site_id=nrsi&id=OFL).

### Fonts installation

1. Create a `.fonts` directory in your home directory: `mkdir ~/.fonts`
2. Download both fonts zip files
3. Extract all `.ttf` files into `~/.fonts`

## Container Runtime

The `remote-core` simulator is available as Docker image. One can either use Docker or Podman as container runtime.  
Let's use Docker for easier Home Assistant setup.  
Follow the official instructions for your distribution: [Ubuntu](https://docs.docker.com/engine/install/ubuntu/),
[Debian](https://docs.docker.com/engine/install/debian/). In short, for Ubuntu:
```bash
sudo apt install apt-transport-https ca-certificates curl software-properties-common
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo gpg --dearmor -o /usr/share/keyrings/docker-archive-keyring.gpg
echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/docker-archive-keyring.gpg] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable" | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null
sudo apt update
sudo apt install docker-ce
# add user to docker group. Log out and back in afterwards
sudo usermod -aG docker ${USER}
```

### Alternative: Podman

```bash
sudo apt install podman
```

If you'd also like the Docker aliases:
```bash
sudo apt install podman-docker
```

## Integrations

The [Remote-Core Simulator](https://github.com/unfoldedcircle/core-simulator) includes a ready to run Docker compose configuration with the Home Assistant integration and server.

The following instructions are for a manual setup.

### Home Assistant

Run the container:
```bash
docker run -d \
  --name uc-intg-hass \
  --restart=unless-stopped \
  -e TZ=UTC \
  docker.io/unfoldedcircle/intg-hass:latest
```

### Home Assistant Server

If you don't already have a [Home Assistant](https://www.home-assistant.io/) installation you can easily install one in
a container to get started with the UC home-assistant integration for Remote Two/3.

Create configuration directory on the host: 
```bash
mkdir ~/hass_config
```

Run the container - replace the `TZ` value with your time zone:
```bash
docker run -d \
  --name homeassistant \
  --privileged \
  --restart=unless-stopped \
  -e TZ=UTC \
  -v ~/hass_config:/config \
  --network=host \
  ghcr.io/home-assistant/home-assistant:stable
```

Open a web browser at <http://localhost:8123> and start the onboarding.  
Afterwards go to your user profile (your name, bottom left) and create a long-lived access token to access the
Home Assistant API with the UC home-assistant integration. Copy and save it in your password manager.

For further information please see: <https://www.home-assistant.io/installation/linux>
