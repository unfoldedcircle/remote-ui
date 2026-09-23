# Cross-compile a static Windows x64 remote-ui app (experimental)

> **Experimental, unsupported.** We develop and test remote-ui on Linux and macOS only. The Windows build exists
> because the cross-compile image made it cheap to try: it compiles, and a first smoke test on a Windows 10 VM
> started the app. Nobody verifies it for releases, and problems with it are not tracked. Use it at your own risk,
> and please report what works and what doesn't.

Part of [compile a static desktop remote-ui app](static-compile.md). The result is a single `remote-ui.exe` for
Windows 10 x64 or newer: Qt, its plugins and QML modules, OpenSSL and the image and font libraries are linked in, only
Windows system DLLs are used. It is cross-compiled **on a Linux host** (or in CI) with the
[MXE](https://mxe.cc) based Docker image `unfoldedcircle/remote-ui-toolchain-qt-5.15.19-static-windows-x64` from the
[ucr2-toolchain](https://github.com/unfoldedcircle/ucr2-toolchain) repository (`docker-windows-x64/`). There is no
native Windows build setup and no Qt Creator kit.

## 1. Build

On a Linux machine with Docker, in the remote-ui checkout:

```bash
make windows-x64          # pulls the image if needed -> binaries/windows-x64/release/remote-ui.exe
```

`make windows-x64 WINDOWS_IMAGE=<image>` selects another image, `make clean-windows-x64` removes the build. The
target reverts the `resources/translations/*.ts` churn of `lupdate`, like the other Docker targets. Doing it by hand:

```bash
docker run --rm --user=$(id -u):$(id -g) -v "$(pwd)":/sources \
    unfoldedcircle/remote-ui-toolchain-qt-5.15.19-static-windows-x64
git checkout -- resources/translations/   # qmake ran lupdate, undo the .ts churn
```

Output: `binaries/windows-x64/release/remote-ui.exe` (about 65 MB, already stripped). Intermediate files go to
`build/windows-x86_64/release-static/`. The container runs `qmake CONFIG+=static CONFIG+=release` and `make`, like
the device and the Linux x64 images (see the image README for a manual build with a shell in the container).
There is no GitHub workflow entry for the Windows build.

What the Windows build changes in the sources: nothing platform-specific is compiled in. `enums.h` undefines the
`ERROR` and `DELETE` macros that `windows.h` (pulled in through `QQuickWindow`) would otherwise inject into the enum
declarations, the UCR3 touch slider (an evdev device) reports itself as unavailable outside Linux, and
`remote-ui.pro` builds the executable for the **console subsystem** if enabled (`CONFIG += console`), see [Logging](#4-logging).

## 2. Graphics: OpenGL or ANGLE

Qt Quick renders with OpenGL. The Qt in the image is configured with `-opengl dynamic`, the same as Qt's official
Windows binaries: at startup Qt tries, in this order,

1. the system `opengl32.dll` (needs a GPU driver with OpenGL 2.0 or newer; fine on physical machines),
2. **ANGLE**, OpenGL ES on top of Direct3D 11, loaded from `libEGL.dll` and `libGLESv2.dll` next to `remote-ui.exe`
   (plus `d3dcompiler_47.dll`, which Windows 10 ships in `System32`),
3. the software rasterizer `opengl32sw.dll` (Mesa llvmpipe) next to the executable.

"Static" does not include ANGLE: Qt always loads it as DLLs, and the image does not build it. In a **virtual machine**
(and on some machines with old drivers) step 1 fails because the OpenGL driver is the Windows 1.1 fallback, and the
app does not start until the ANGLE DLLs are placed **in the same directory as `remote-ui.exe`**. The Qt Quick
software renderer (`QT_QUICK_BACKEND=software`) is not an option: the UI uses `QtGraphicalEffects` shaders, which it
cannot draw.

Where to get the DLLs:

- **[mmozeiko/build-angle](https://github.com/mmozeiko/build-angle/releases)**: weekly builds of upstream ANGLE.
  Download `angle-x64-<date>.zip`, copy `bin/libEGL.dll`, `bin/libGLESv2.dll` and `bin/d3dcompiler_47.dll` next to
  `remote-ui.exe`. This is what the smoke test used. Simplest option: a zip, no installer, no Qt account.
- **Qt's own ANGLE**, the version Qt 5.15 was developed with: the `libEGL.dll` and `libGLESv2.dll` in the `bin/`
  directory of any Qt 5.15.2 Windows binary package (Qt online installer, or on any OS
  `aqt install-qt windows desktop 5.15.2 win64_mingw81 --archives qtbase d3dcompiler_47`, see
  [install-debian-13.md](install-debian-13.md) for `aqtinstall`). Prefer this if the current ANGLE misbehaves.
- The **software fallback** instead of ANGLE: `opengl32sw.dll` from the same Qt package (`--archives opengl32sw`)
  next to the executable, with `QT_OPENGL=software`. Slow but driver-independent.

`QT_OPENGL=angle` forces ANGLE, `QT_OPENGL=desktop` forces the system OpenGL; the start script below leaves the
choice to Qt. Nothing else is installed: no Visual C++ runtime (MinGW build) and no OpenSSL DLLs (linked in).

## 3. Run

The app reads the same environment variables as on Linux (`README.md`, "Environment Variables") and needs a
running [Remote-Core Simulator](install.md#container-runtime). `scripts/env/windows.cmd` sets the defaults and starts
the executable:

```bat
scripts\env\windows.cmd                       :: remote-ui.exe from binaries\windows-x64\release
scripts\env\windows.cmd C:\path\to\remote-ui.exe
```

Every variable is a default: set it before calling the script to override it, e.g.
`set UC_TOKEN_PATH=C:\core-simulator\docker\ui-env\ws-token` (the simulator's access token; the script's default
expects a `core-simulator` checkout next to `remote-ui`), `set UC_SOCKET_URL=ws://192.168.1.10:8080/ws` for a
simulator on another machine, `set UC_DISPLAY_SCALE=0.5` on a display scaled to 200 %. The script keeps the console
window open when the app exits with an error, so the log stays readable; Qt's log goes to that console.

The [fonts](install.md#fonts) (Poppins, Space Mono) are needed like on Linux: install them for the user
(right click, "Install") before starting the app.

## 4. Logging

The executable can be built for the console subsystem, so Qt's log output (`qCDebug` etc., the same categories as on
Linux) appears in the command window the app was started from, and a console window opens next to the app when
`remote-ui.exe` is double-clicked. `QT_LOGGING_RULES` works as on Linux, e.g. `set QT_LOGGING_RULES=uc.core.debug=false`.

Enable console logging with: `CONFIG += console` in `remote-ui.pro`.

Background: a Windows *GUI* subsystem application has no usable stderr, even when started from a command shell, and
Qt then sends its messages to the debugger only (visible with Sysinternals DebugView, or by forcing stderr with
`QT_FORCE_STDERR_LOGGING=1` after attaching a console). The console subsystem avoids all of that for a developer
build.

## Known limitations and open points

- Experimental: not verified with the core simulator yet, not tested on physical Windows machines, no CI build.
- The `DEV` model opens the button simulator as a second window like on Linux; keyboard focus behaviour is untested.
- The Windows build has no application icon and no version resource yet (`win32:RC_ICONS` in `remote-ui.pro`).
- Sound effects use Qt Multimedia's Windows backend (Media Foundation), untested.
- The image is 1.5 GB (302 MB to pull) after removing MXE's host tools and the dependency packages' Windows
  executables; the remaining static libraries include some remote-ui never links.
