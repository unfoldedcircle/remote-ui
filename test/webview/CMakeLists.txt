project (testWebView LANGUAGES CXX)

include_directories(
    ../../src
    ../..
)

find_package(QT NAMES Qt5 Qt6 COMPONENTS Core Gui Qml Quick Test REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Gui Qml Quick Test REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing(true)

# Qt key -> XKB keysym table, modifier bits, touch phases and the IME hint mapping (pure functions)
add_executable (testWebViewInput
    "test_webview_input.cpp"
    "../../src/webview/webviewinput.cpp"
    )

add_test(NAME testWebViewInput
         COMMAND testWebViewInput)

target_link_libraries(testWebViewInput
    PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Test
    )

# the pending/current/retired frame bookkeeping between the GUI and the render thread (pure state machine)
add_executable (testWebViewFrameQueue
    "test_webview_frame_queue.cpp"
    "../../src/webview/webviewframequeue.cpp"
    )

add_test(NAME testWebViewFrameQueue
         COMMAND testWebViewFrameQueue)

target_link_libraries(testWebViewFrameQueue
    PUBLIC
    Qt5::Core
    Qt5::Test
    )

# the WebView item against a stubbed libucwebview function table: lifecycle, load events, crash recovery, input
# method contract and key forwarding, without the library and without rendering (offscreen platform)
add_executable (testWebViewItem
    "test_webview_item.cpp"
    "../../src/webview/webviewitem.cpp"
    "../../src/webview/webviewlibrary.cpp"
    "../../src/webview/webviewinput.cpp"
    "../../src/webview/webviewframequeue.cpp"
    "../../src/webview/webviewtexture.cpp"
    "../../src/logging.cpp"
    )

add_test(NAME testWebViewItem
         COMMAND testWebViewItem)

target_link_libraries(testWebViewItem
    PUBLIC
    Qt5::Core
    Qt5::Gui
    # <qpa/qplatformnativeinterface.h> for the EGLDisplay lookup in webviewlibrary.cpp
    Qt5::GuiPrivate
    Qt5::Quick
    Qt5::Test
    )

# the on-screen keyboard on the WebView settings page: the page as the app loads it under a copy of main.qml's
# keyboard controller, with the Qt Virtual Keyboard input context and the real InputController (offscreen platform)
add_executable (testWebViewKeyboard
    "test_webview_keyboard.cpp"
    "test_webview_keyboard.qrc"
    "../../resources/qrc/main.qrc"
    "../../src/webview/webviewitem.cpp"
    "../../src/webview/webviewlibrary.cpp"
    "../../src/webview/webviewinput.cpp"
    "../../src/webview/webviewframequeue.cpp"
    "../../src/webview/webviewtexture.cpp"
    "../../src/ui/inputController.cpp"
    "../../src/ui/colors.cpp"
    # header only: contains the Q_OBJECT / Q_GADGET classes moc has to process
    "../../src/ui/fonts.h"
    "../../src/core/enums.h"
    "../../src/hardware/hardwareModel.h"
    "../../src/logging.cpp"
    )

add_test(NAME testWebViewKeyboard
         COMMAND testWebViewKeyboard)

target_link_libraries(testWebViewKeyboard
    PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::GuiPrivate
    Qt5::Qml
    Qt5::Quick
    Qt5::Test
    )
