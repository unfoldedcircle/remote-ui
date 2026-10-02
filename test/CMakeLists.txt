cmake_minimum_required(VERSION 3.15)

# No CMAKE_OSX_ARCHITECTURES in the test CMakeLists: on macOS the tests are built for the host architecture, like the
# Qt built from source (docs/static-compile-macos.md). For an x86_64-only Qt on an Apple Silicon Mac, configure with
# -D CMAKE_OSX_ARCHITECTURES=x86_64 (or set that environment variable for a new build directory).
enable_testing()
project(remote-ui_tests LANGUAGES CXX)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Quick REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Quick REQUIRED)

add_subdirectory(common)
add_subdirectory(core)
add_subdirectory(hardware)
add_subdirectory(i18n)
add_subdirectory(ui)
