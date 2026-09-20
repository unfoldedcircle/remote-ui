project (testCore LANGUAGES CXX)

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

# the response handler templates use generic lambdas
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Mostly for developing on macOS with an M1 or newer CPU. Old Qt version isn't available as arm64
set(CMAKE_OSX_ARCHITECTURES x86_64)

enable_testing(true)

add_executable (testCore
    "test_core_response_handlers.cpp"
    "../../src/core/core.cpp"
    # header only: contains the Q_GADGET enums moc has to process
    "../../src/core/enums.h"
    "../../src/ui/notification.cpp"
    "../../src/util.cpp"
    "../../src/logging.cpp"
    "../../3rd-party/QR-Code-generator/cpp/qrcodegen.cpp"
    )

add_test(NAME testCore
         COMMAND testCore)

target_link_libraries(testCore
    PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Qml
    Qt5::WebSockets
    Qt5::Test
    )
