project (testHardware LANGUAGES CXX)

include_directories(
    ../../src
    ../..
    ../../3rd-party/QR-Code-generator/cpp/
)

find_package(QT NAMES Qt5 Qt6 COMPONENTS Gui Qml WebSockets Test REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Gui Qml WebSockets Test REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Mostly for developing on macOS with an M1 or newer CPU. Old Qt version isn't available as arm64
set(CMAKE_OSX_ARCHITECTURES x86_64)

enable_testing(true)

add_executable (testHardware
    "test_touchslider_eventfilter.cpp"
    "../../src/hardware/ucr3/touchSliderEventFilter.cpp"
    )

add_test(NAME testHardware
         COMMAND testHardware)

target_link_libraries(testHardware
    PUBLIC
    Qt5::Core
    Qt5::Test
    )

add_executable (testBattery
    "test_battery.cpp"
    "../../src/hardware/battery.cpp"
    "../../src/core/core.cpp"
    # header only: contains the Q_GADGET enums moc has to process
    "../../src/core/enums.h"
    "../../src/ui/notification.cpp"
    "../../src/util.cpp"
    "../../src/logging.cpp"
    "../../3rd-party/QR-Code-generator/cpp/qrcodegen.cpp"
    )

# the core response handler templates use generic lambdas
set_property(TARGET testBattery PROPERTY CXX_STANDARD 17)
set_property(TARGET testBattery PROPERTY CXX_STANDARD_REQUIRED ON)

add_test(NAME testBattery
         COMMAND testBattery)

target_link_libraries(testBattery
    PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Qml
    Qt5::WebSockets
    Qt5::Test
    )
