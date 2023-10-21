# Compile a static desktop remote-ui app

## macOS

### Install Qt 5.15.2 & Qt Creator

1. Download Qt for Open Source development: <https://www.qt.io/download>
2. Register, if you don't have an account yet.
3. Install:
   - Use default installation folder: `~/Qt`
   - Select:
     - Qt 5.15.2
     - Sources
     - Qt Virtual Keyboard
     - Qt Network Authorization

### Compile static Qt framework

1. Make sure that XCode and command line tools are installed
2. Patch Qt source file `~/Qt/5.15.2/Src/qtbase/src/plugins/platforms/cocoa/qiosurfacegraphicsbuffer.h`  
   Add the following include just before QT_BEGIN_NAMESPACE:
```
#include <CoreGraphics/CGColorSpace.h>
```

3. Create a shallow build directory (don't compile inside source directory):
```shell
mkdir -p ~/projects/qt-static-5.15.2
cd ~/projects/qt-static-5.15.2
```
4. Configure
```shell
~/Qt/5.15.2/Src/configure --prefix=~/Qt/5.15.2/clang_64-static -static -debug-and-release \
    -nomake examples -nomake tests \
    -qt-libpng -qt-zlib -qt-libjpeg \
    -opensource -confirm-license \
    -opengl -qt-freetype -qt-pcre -qt-harfbuzz -platform macx-clang
```
5. Make  
   Adjust `4` to the available number of cores in your machine:
```shell
make -j 4
```
6. Install
```shell
make -j 4 install
```
7. Configure a new Qt Kit in Qt Creator
   - Preferences, Kits, Qt Versions: Add...
     - Select `~/Qt/5.15.2/clang_64-static/bin/qmake `
   - Preferences, Kits: Add
     - Name: Desktop Qt 5.15.2 static
     - Qt version: previously defined Qt version `Qt 5.15.2 (clang_64-static)`
8. Change Kit in project, compile, finished!

## Linux

Contributions welcomed 😊

## Windows

Contributions welcomed 😊
