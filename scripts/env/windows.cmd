@echo off
:: Runtime environment for the Windows x64 desktop simulator, cross-compiled with the Docker toolchain image
:: (docs/static-compile-windows.md, experimental). Starts remote-ui.exe with the default settings:
::   scripts\env\windows.cmd                          -> binaries\windows-x64\release\remote-ui.exe
::   scripts\env\windows.cmd C:\path\to\remote-ui.exe
:: Every value is a default: set the variable before calling the script to override it.
:: App variables: README.md, "Environment Variables".
setlocal

:: Repository root (this script lives in scripts\env)
set "ROOT=%~dp0..\.."
set "EXE=%~1"
if "%EXE%"=="" set "EXE=%ROOT%\binaries\windows-x64\release\remote-ui.exe"
if not exist "%EXE%" (
    echo remote-ui.exe not found: %EXE%
    echo Build it as described in docs\static-compile-windows.md or pass the path as first argument.
    exit /b 1
)

:: Desktop simulator settings
if not defined UC_MODEL set "UC_MODEL=DEV"
if not defined UC_DISPLAY_WIDTH set "UC_DISPLAY_WIDTH=480"
if not defined UC_DISPLAY_HEIGHT set "UC_DISPLAY_HEIGHT=850"
:: 1 is the app default on Windows (regular display); use 0.5 on a display scaled to 200 %%.
if not defined UC_DISPLAY_SCALE set "UC_DISPLAY_SCALE=1"
:: Remote-Core Simulator (https://github.com/unfoldedcircle/core-simulator): WebSocket URL and access token file
if not defined UC_SOCKET_URL set "UC_SOCKET_URL=ws://127.0.0.1:8080/ws"
if not defined UC_TOKEN_PATH set "UC_TOKEN_PATH=%ROOT%\..\core-simulator\docker\ui-env\ws-token"

:: Graphics: Qt picks the system OpenGL, then ANGLE (libEGL.dll + libGLESv2.dll next to remote-ui.exe), then
:: opengl32sw.dll. Force one with QT_OPENGL=desktop | angle | software, see docs\static-compile-windows.md.

echo Starting %EXE%
echo   UC_MODEL=%UC_MODEL%  UC_DISPLAY_WIDTH=%UC_DISPLAY_WIDTH%  UC_DISPLAY_HEIGHT=%UC_DISPLAY_HEIGHT%  UC_DISPLAY_SCALE=%UC_DISPLAY_SCALE%
echo   UC_SOCKET_URL=%UC_SOCKET_URL%  UC_TOKEN_PATH=%UC_TOKEN_PATH%
if defined QT_OPENGL echo   QT_OPENGL=%QT_OPENGL%
"%EXE%"
if errorlevel 1 (
    echo.
    echo remote-ui exited with error %errorlevel%. If it did not start in a virtual machine, put the ANGLE DLLs
    echo next to remote-ui.exe, see docs\static-compile-windows.md.
    pause
)
endlocal
