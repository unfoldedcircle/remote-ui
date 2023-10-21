# Remote Two Cross-Compile & Installation 

## Cross-Compile

The easiest way to cross-compile for the Remote Two device is with our prepared toolchain Docker image:

```bash
docker pull unfoldedcircle/r2-toolchain-qt-5.15.8-static:latest
docker run --rm  \
    --user=$(id -u):$(id -g) \
    -v $(pwd):/sources \
    unfoldedcircle/r2-toolchain-qt-5.15.8-static:latest
```

ℹ️ The included GitHub build action can be used to create a cross-compiled binary.  
Enable the GitHub actions in your cloned repository and it will be built automatically for all pull requests and SemVer version tags.

_TODO_ output binary

## Install custom version on Remote Two

☢️ VOIDS WARRANTY ☢️

⚠️ **Warning:**
- **Installing a custom remote-ui version on the Remote Two device will void your warranty!**
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
