QT += qml quick websockets quickcontrols2 multimedia network

static {
    QTPLUGIN += qtvirtualkeyboardplugin
    QT += svg
}

macx {
    CONFIG+=sdk_no_version_check
    # There seems no other way to change the taskbar & app switcher name
    TARGET = "Remote UI"
} else {
    TARGET = remote-ui
}

CONFIG += c++17 disable-desktop handwriting
CONFIG += qtquickcompiler

QMAKE_CXXFLAGS_WARN_ON += \
    -Wold-style-cast \
    -Wfloat-equal \
    -Woverloaded-virtual \
    -Wshadow

DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += QT_MESSAGELOGCONTEXT

macx {
    # Ensure OpenGL is linked correctly (pair stays together).
    QMAKE_LIBS_OPENGL = -framework OpenGL

    # Strip AGL from any places Qt might add it.
    QMAKE_MAC_FRAMEWORKS -= AGL
    LIBS -= -framework AGL
}

#DEFINES += "TEST_MODE"

# === Version and build information ===========================================
# Qt VERSION variable is too restrictive: major, minor, patch level, and build
# number must be in the range from 0 to 65535.
# https://doc.qt.io/qt-5/qmake-variable-reference.html#version

# If built in Buildroot use custom package version, otherwise Git
isEmpty(UC_BUILD_VERSION) {
    GIT_VERSION = "$$system(git describe --match "v[0-9]*" --tags HEAD --always)"
    GIT_HASH = "$$system(git log -1 --format="%H")"
    GIT_BRANCH = "$$system(git rev-parse --abbrev-ref HEAD)"
} else {
    GIT_VERSION = $$UC_BUILD_VERSION
    contains(GIT_VERSION, "^v?(0|[1-9]\d*)\..*") {
        # (simplified) version string = regular release
        GIT_HASH = ""
        GIT_BRANCH = "main"
    } else {
        # git hash as version = custom build
        GIT_HASH = $$UC_BUILD_VERSION
        GIT_BRANCH = ""
    }
}
REMOTE_VERSION = $$replace(GIT_VERSION, v, "")
DEFINES += APP_VERSION=\\\"$$REMOTE_VERSION\\\"

# TODO map REMOTE_VERSION to VERSION if format is X.Y.Z
VERSION = 0.1.0

# build timestamp
BUILDDATE=$$system(date +"%Y-%m-%dT%H:%M:%S")
# =============================================================================

CONFIG(debug, debug|release) {
    DEBUG_BUILD = true
} else {
    DEBUG_BUILD = false
}

QMAKE_SUBSTITUTES += version.txt.in

HEADERS += \
    src/config/config.h \
    src/core/core.h \
    src/core/enums.h \
    src/core/structs.h \
    src/dock/configuredDocks.h \
    src/dock/discoveredDocks.h \
    src/dock/dockController.h \
    src/hardware/battery.h \
    src/hardware/haptic.h \
    src/hardware/hardwareController.h \
    src/hardware/info.h \
    src/hardware/power.h \
    src/hardware/touchSlider.h \
    src/hardware/ucr2/hapticUCR2.h \
    src/hardware/ucr2/hapticUCR3.h \
    src/hardware/ucr3/touchSliderUCR3.h \
    src/hardware/wifi.h \
    src/integration/confirmationPage.h \
    src/integration/integrationController.h \
    src/integration/integrationDrivers.h \
    src/integration/integrations.h \
    src/integration/setupSchema.h \
    src/logging.h \
    src/hardware/hardwareModel.h \
    src/softwareupdate/softwareUpdate.h \
    src/translation/translation.h \
    src/ui/colors.h \
    src/ui/entity/availableEntities.h \
    src/ui/entity/configuredEntities.h \
    src/ui/entity/entities.h \
    src/ui/entity/activity.h \
    src/ui/entity/button.h \
    src/ui/entity/climate.h \
    src/ui/entity/cover.h \
    src/ui/entity/entitiesProxy.h \
    src/ui/entity/entity.h \
    src/ui/entity/light.h \
    src/ui/entity/macro.h \
    src/ui/entity/mediaPlayer.h \
    src/ui/entity/remote.h \
    src/ui/entity/sensor.h \
    src/ui/entity/sequenceStep.h \
    src/ui/entity/switch.h \
    src/ui/entity/entityController.h \
    src/ui/fonts.h \
    src/ui/group/group.h \
    src/ui/group/groupController.h \
    src/ui/group/groupItem.h \
    src/ui/inputController.h \
    src/ui/notification.h \
    src/ui/onboardingController.h \
    src/ui/page/page.h \
    src/ui/page/pageItem.h \
    src/ui/page/pages.h \
    src/ui/profile/profile.h \
    src/ui/profile/profiles.h \
    src/ui/resources.h \
    src/ui/soundEffects.h \
    src/ui/uiController.h \
    src/util.h \
    src/voice.h


SOURCES += \
        src/config/config.cpp \
        src/core/core.cpp \
        src/dock/configuredDocks.cpp \
        src/dock/discoveredDocks.cpp \
        src/dock/dockController.cpp \
        src/hardware/battery.cpp \
        src/hardware/haptic.cpp \
        src/hardware/hardwareController.cpp \
        src/hardware/info.cpp \
        src/hardware/power.cpp \
        src/hardware/touchSlider.cpp \
        src/hardware/ucr2/hapticUCR2.cpp \
        src/hardware/ucr2/hapticUCR3.cpp \
        src/hardware/ucr3/touchSliderUCR3.cpp \
        src/hardware/wifi.cpp \
        src/integration/confirmationPage.cpp \
        src/integration/setupSchema.cpp \
        src/logging.cpp \
        src/main.cpp \
        src/softwareupdate/softwareUpdate.cpp \
        src/translation/translation.cpp \
        src/ui/colors.cpp \
        src/ui/entity/availableEntities.cpp \
        src/ui/entity/configuredEntities.cpp \
        src/ui/entity/entities.cpp \
        src/ui/entity/activity.cpp \
        src/ui/entity/button.cpp \
        src/ui/entity/climate.cpp \
        src/ui/entity/cover.cpp \
        src/ui/entity/entitiesProxy.cpp \
        src/ui/entity/entity.cpp \
        src/ui/entity/light.cpp \
        src/ui/entity/macro.cpp \
        src/ui/entity/mediaPlayer.cpp \
        src/ui/entity/remote.cpp \
        src/ui/entity/sensor.cpp \
        src/ui/entity/switch.cpp \
        src/ui/entity/entityController.cpp \
        src/ui/group/group.cpp \
        src/ui/group/groupController.cpp \
        src/ui/group/groupItem.cpp \
        src/ui/inputController.cpp \
        src/integration/integrationController.cpp \
        src/integration/integrationDrivers.cpp \
        src/integration/integrations.cpp \
        src/ui/notification.cpp \
        src/ui/onboardingController.cpp \
        src/ui/page/page.cpp \
        src/ui/page/pageItem.cpp \
        src/ui/page/pages.cpp \
        src/ui/profile/profiles.cpp \
        src/ui/resources.cpp \
        src/ui/soundEffects.cpp \
        src/ui/uiController.cpp \
        src/util.cpp \
        src/voice.cpp

# === QR code generator =======================================
HEADERS += \
        3rd-party/QR-Code-generator/cpp/qrcodegen.hpp

SOURCES += \
        3rd-party/QR-Code-generator/cpp/qrcodegen.cpp

# === resources =======================================
RESOURCES += resources/qrc/main.qrc \
    resources/qrc/button-simulator.qrc \
    resources/qrc/icons.qrc \
    resources/qrc/images.qrc \
    resources/qrc/translations.qrc \
    resources/qrc/keyboard.qrc

# turn off resource compression
QMAKE_RESOURCE_FLAGS += -no-compress

macx {
    # custom plist file for pretty app name etc
    QMAKE_INFO_PLIST = $$PWD/resources/mac/Info.plist
}

# === start TRANSLATION section =======================================
TRANSLATIONS += resources/translations/da_DK.ts \
               resources/translations/de_DE.ts \
               resources/translations/de_CH.ts \
               resources/translations/en_US.ts \
               resources/translations/hu_HU.ts \
               resources/translations/nl_NL.ts \
               resources/translations/fr_FR.ts \
               resources/translations/it_IT.ts

# -----------------------------------------------------------------------------
# Qt Linguist tools
# -----------------------------------------------------------------------------
# lupdate & lrelease integration in qmake is a major pain to get working on Linux, macOS, Windows PLUS Linux arm cross compile PLUS qmake / make cmd line!
# There are so many different ways and each one works great on SOME platform(s) only :-(
# So this here might look excessive but I found no other reliable way to make it work on as many environments as possible...
# 1.) Check if we get the linguist cmd line tools from the QT installation (works in Qt Creator on Linux, macOS and Win but not with Buildroot / Linux crosscompile)
exists($$[QT_INSTALL_BINS]/lupdate):QMAKE_LUPDATE = $$[QT_INSTALL_BINS]/lupdate
exists($$[QT_INSTALL_BINS]/lrelease):QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
  # think about our Windows friends
exists($$[QT_INSTALL_BINS]/lupdate.exe):QMAKE_LUPDATE = $$[QT_INSTALL_BINS]/lupdate.exe
exists($$[QT_INSTALL_BINS]/lrelease.exe):QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease.exe
# 2.) Check if it's available from $HOST_DIR env var which is set during Buildroot. Only use it if it's not already defined.
isEmpty(QMAKE_LUPDATE):exists($$(HOST_DIR)/bin/lupdate):QMAKE_LUPDATE = $$(HOST_DIR)/bin/lupdate
isEmpty(QMAKE_LRELEASE):exists($$(HOST_DIR)/bin/lrelease):QMAKE_LRELEASE = $$(HOST_DIR)/bin/lrelease
# 3.) Linux Qt Creator arm cross compile: QT_INSTALL_BINS is NOT available, but host tools should be available in QTDIR
isEmpty(QMAKE_LUPDATE):exists($$(QTDIR)/bin/lupdate):QMAKE_LUPDATE = $$(QTDIR)/bin/lupdate
isEmpty(QMAKE_LRELEASE):exists($$(QTDIR)/bin/lrelease):QMAKE_LRELEASE = $$(QTDIR)/bin/lrelease
# 4.) Fallback: custom env var QT_LINGUIST_DIR (which can also be used to override the tools found in the path)
isEmpty(QMAKE_LUPDATE):exists($$(QT_LINGUIST_DIR)/lupdate):QMAKE_LUPDATE = $$(QT_LINGUIST_DIR)/lupdate
isEmpty(QMAKE_LRELEASE):exists($$(QT_LINGUIST_DIR)/lrelease):QMAKE_LRELEASE = $$(QT_LINGUIST_DIR)/lrelease
# 5.) Last option: check path, plain and simple. (Would most likely be enough on most systems, except Ubuntu with an incomplete Qt installation where it's a symlink to qtchooser...)
if(isEmpty(QMAKE_LUPDATE)) {
    win32:QMAKE_LUPDATE    = $$system(where lupdate)
    unix|mac:QMAKE_LUPDATE = $$system(which lupdate)
}
if(isEmpty(QMAKE_LRELEASE)) {
    win32:QMAKE_LRELEASE    = $$system(where lrelease)
    unix|mac:QMAKE_LRELEASE = $$system(which lrelease)
}


!isEmpty(QMAKE_LUPDATE):exists("$$QMAKE_LUPDATE") {
    message("Using Qt linguist tools: '$$QMAKE_LUPDATE', '$$QMAKE_LRELEASE'")
    command = $$QMAKE_LUPDATE remote-ui.pro
    system($$command) | error("Failed to run: $$command")
    command = $$QMAKE_LRELEASE remote-ui.pro
    system($$command) | error("Failed to run: $$command")
} else {
    warning("Qt linguist cmd line tools lupdate / lrelease not found: translations will NOT be compiled and build will most likely fail due to missing .qm files!")
}

# === end TRANSLATION section =========================================
# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# === start CONFIG COPY section =======================================

macx {
    ICON = $$PWD/resources/mac/UC.icns
}
# === end CONFIG COPY section =======================================

DISTFILES += \
    version.txt.in

# -----------------------------------------------------------------------------
# DESTINATION PATH
# -----------------------------------------------------------------------------

platform_path = unknown-platform
processor_path = $${QT_ARCH}
build_path = unknown-build

macx {
    platform_path = osx
}
linux {
    platform_path = linux
}

BUILD_DEBUG {
    build_path = debug
} else {
    build_path = release
}

DESTINATION_PATH = $${platform_path}-$${processor_path}/$$build_path
message(Dest path: $${DESTINATION_PATH})

# Configure destination path
DESTDIR = $$(UC_BIN)
DESTDIR_BIN = $$(UC_BIN)
isEmpty(DESTDIR) {
    DESTDIR = $$(UC_SRC)
    isEmpty(DESTDIR) {
        DESTDIR_BIN = $$clean_path($$PWD/binaries)
        DESTDIR = $$DESTDIR_BIN/$$DESTINATION_PATH
        message(Environment variables UC_BIN and UC_SRC not defined! Using '$$DESTDIR' as binary output directory.)
    } else {
        DESTDIR_BIN = $$clean_path($$(UC_SRC)/binaries)
        DESTDIR = $$DESTDIR_BIN/$$DESTINATION_PATH
        message(UC_SRC is set: using '$$DESTDIR' as binary output directory.)
    }
} else {
    message(UC_BIN defined '$$DESTDIR' as binary output directory.)
}

OBJECTS_DIR = $$PWD/build/$$DESTINATION_PATH/obj
MOC_DIR = $$PWD/build/$$DESTINATION_PATH/moc
RCC_DIR = $$PWD/build/$$DESTINATION_PATH/qrc
UI_DIR = $$PWD/build/$$DESTINATION_PATH/ui

