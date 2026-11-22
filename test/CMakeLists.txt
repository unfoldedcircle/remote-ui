cmake_minimum_required(VERSION 3.15)

# No CMAKE_OSX_ARCHITECTURES in the test CMakeLists: on macOS the tests are built for the host architecture, like the
# Qt built from source (docs/static-compile-macos.md). For an x86_64-only Qt on an Apple Silicon Mac, configure with
# -D CMAKE_OSX_ARCHITECTURES=x86_64 (or set that environment variable for a new build directory).
enable_testing()
project(remote-ui_tests LANGUAGES CXX)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Quick REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Quick REQUIRED)

# The app build generates version.h (APP_VERSION) with qmake from version.h.in into its build directory; the tests
# that compile translation.cpp, softwareUpdate.h or config.h get a fixed one from here.
file(WRITE "${CMAKE_BINARY_DIR}/generated/version.h" "#pragma once\n#define APP_VERSION \"test\"\n")
include_directories("${CMAKE_BINARY_DIR}/generated")

add_subdirectory(common)
add_subdirectory(core)
add_subdirectory(hardware)
add_subdirectory(i18n)
add_subdirectory(ui)
add_subdirectory(webview)
