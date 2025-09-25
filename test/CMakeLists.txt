cmake_minimum_required(VERSION 3.15)

enable_testing()
project(remote-ui_tests LANGUAGES CXX)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Quick REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Quick REQUIRED)

add_subdirectory(common)
