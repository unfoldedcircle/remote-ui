# Remote Two/3 Cross-Compile & Installation 

## Cross-Compile

The easiest way to cross-compile for the Remote Two/3 devices is with our prepared toolchain Docker image:

```bash
make ucr2
```

The target pulls the image if it is missing, runs it exactly like the GitHub workflow does, writes `version.txt`
next to the binary and reverts the `resources/translations/*.ts` churn of the toolchain's `lupdate` (only for files
that were unmodified before). Run `docker pull` yourself to update the image, `make clean-ucr2` to start from
scratch; `make` without a target lists all targets.

Doing it by hand:

```bash
docker pull unfoldedcircle/r2-toolchain-qt-5.15.8-static:latest
docker run --rm  \
    --user=$(id -u):$(id -g) \
    -v $(pwd):/sources \
    unfoldedcircle/r2-toolchain-qt-5.15.8-static:latest
```

The container runs `qmake CONFIG+=static CONFIG+=release` and `make`; the binary lands in the default output path of
`remote-ui.pro`, `binaries/linux-arm64/release/` (the workflow and the Makefile rely on that path, do not change it).

ℹ️ The included GitHub build action can be used to create a cross-compiled binary.  
Enable the GitHub actions in your cloned repository and it will be built automatically for all pull requests and SemVer version tags.

The output binary is stored in `binaries/linux-arm64/release/` on the host.

## Install custom version on Remote Two/3

☢️ VOIDS WARRANTY ☢️

⚠️ **Warning:**
- **Installing a custom remote-ui version on the Remote Two/3 devices will void your warranty!**
- Only intended for developers and power users
- Do not install custom binaries from untrusted sources!    

### Installation Archive

Custom archive requirements:
- TAR GZip archive (either .tgz or .tar.gz file suffix).
- In the root of the archive, there must be a `release.json` file describing the custom version.  
  See `CustomRelease` schema of the Core-API for the release.json format.
- No symlinks. They are automatically removed during the installation.
- The UI binary must be named `remote-ui` in the `./bin` subdirectory.
- All application files must be in one of the following subdirectories, other locations are not accessible at runtime:
  - `./bin`: application binary, usually only `remote-ui`.
  - `./config`: configuration data. Path is accessible with `UC_CONFIG_HOME` environment variable.
  - `./data`: application data. Path is accessible with `UC_DATA_HOME` environment variable.

### Installation

```console
curl --location 'http://$IP/api/system/install/ui?void_warranty=$CONFIRMATION' \
--form 'file=@"$INSTALL_ARCHIVE"' \
-u 'web-configurator:$PIN'
```

- Installing a custom remote-ui will restart the application and the screen will go dark for a while.
- If the custom remote-ui app cannot be started, the device will automatically switch back to the factory version after a few attempts.
- For certain error conditions, the device might restart.
