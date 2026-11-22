project (testWebView LANGUAGES CXX)

include_directories(
    ../../src
    ../..
    ../../3rd-party/QR-Code-generator/cpp/
)

find_package(QT NAMES Qt5 Qt6 COMPONENTS Core Gui Qml Quick WebSockets Test REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Gui Qml Quick WebSockets Test REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing(true)

# Qt key -> XKB keysym table, the keys of the page by name, modifier bits, touch phases and the IME hint mapping (pure
# functions)
add_executable (testWebViewInput
    "test_webview_input.cpp"
    "../../src/webview/webviewinput.cpp"
    # the key names of the button mapping (core-api WebviewButtonKey)
    "../../src/ui/entity/webviewButtons.cpp"
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

# the pinch zoom's picture, hold and commits and the node geometry (pure state machine and arithmetic)
add_executable (testWebViewPinch
    "test_webview_pinch.cpp"
    "../../src/webview/webviewpinch.cpp"
    )

add_test(NAME testWebViewPinch
         COMMAND testWebViewPinch)

target_link_libraries(testWebViewPinch
    PUBLIC
    Qt5::Core
    Qt5::Test
    )

# the page's requests (library 0.7.0): the request objects and their table against stubbed reply functions
add_executable (testWebViewRequests
    "test_webview_requests.cpp"
    "../../src/webview/webviewrequest.cpp"
    "../../src/logging.cpp"
    )

add_test(NAME testWebViewRequests
         COMMAND testWebViewRequests)

target_link_libraries(testWebViewRequests
    PUBLIC
    Qt5::Core
    Qt5::Test
    )

# the device state for web pages (library 0.9.0): the state per power mode and battery, the standby events with the time
# slept (a fake clock), the resume flag before and after the mode change (pure state machine)
add_executable (testWebViewDeviceState
    "test_webview_device_state.cpp"
    "../../src/webview/devicestatesource.cpp"
    "../../src/logging.cpp"
    )

add_test(NAME testWebViewDeviceState
         COMMAND testWebViewDeviceState)

target_link_libraries(testWebViewDeviceState
    PUBLIC
    Qt5::Core
    Qt5::Test
    )

# the WebView item against a stubbed libucwebview function table: lifecycle, load events, crash recovery, input
# method contract and key forwarding, without the library and without rendering (offscreen platform)
add_executable (testWebViewItem
    "test_webview_item.cpp"
    "../../src/webview/webviewitem.cpp"
    "../../src/webview/devicestatesource.cpp"
    "../../src/webview/webviewlibrary.cpp"
    "../../src/webview/webviewinput.cpp"
    "../../src/webview/webviewframequeue.cpp"
    "../../src/webview/webviewpinch.cpp"
    "../../src/webview/webviewrequest.cpp"
    "../../src/webview/webviewsite.cpp"
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
    # the spinner of the zoom label (loader_small.png)
    "../../resources/qrc/images.qrc"
    "../../src/webview/webviewitem.cpp"
    "../../src/webview/devicestatesource.cpp"
    "../../src/webview/webviewlibrary.cpp"
    "../../src/webview/webviewinput.cpp"
    "../../src/webview/webviewframequeue.cpp"
    "../../src/webview/webviewpinch.cpp"
    "../../src/webview/webviewrequest.cpp"
    "../../src/webview/webviewsite.cpp"
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

# the keys of the fullscreen web view of a web view entity: its screen as main.qml loads it, on its own and on top of
# an activity screen, for a real web view entity, with the real InputController (offscreen platform)
add_executable (testWebViewEntityScreen
    "test_webview_entity_screen.cpp"
    "test_webview_entity_screen.qrc"
    "../../resources/qrc/main.qrc"
    # the spinner of the zoom label (loader_small.png)
    "../../resources/qrc/images.qrc"
    "../../src/webview/webviewitem.cpp"
    "../../src/webview/devicestatesource.cpp"
    "../../src/webview/webviewlibrary.cpp"
    "../../src/webview/webviewinput.cpp"
    "../../src/webview/webviewframequeue.cpp"
    "../../src/webview/webviewpinch.cpp"
    "../../src/webview/webviewrequest.cpp"
    "../../src/webview/webviewsite.cpp"
    "../../src/webview/webviewtexture.cpp"
    "../../src/ui/entity/webview.cpp"
    "../../src/ui/entity/webviewButtons.cpp"
    "../../src/ui/entity/entity.cpp"
    "../../src/ui/inputController.cpp"
    "../../src/ui/colors.cpp"
    # header only: contains the Q_OBJECT / Q_GADGET classes moc has to process
    "../../src/ui/fonts.h"
    "../../src/core/enums.h"
    "../../src/hardware/hardwareModel.h"
    "../../src/util.cpp"
    "../../src/logging.cpp"
    "../../3rd-party/QR-Code-generator/cpp/qrcodegen.cpp"
    )

add_test(NAME testWebViewEntityScreen
         COMMAND testWebViewEntityScreen)

target_link_libraries(testWebViewEntityScreen
    PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::GuiPrivate
    Qt5::Qml
    Qt5::Quick
    Qt5::Test
    )

# HOME on a fullscreen web view on top of another screen: the real main.qml with its containers and screens, a real web
# view entity without the web view library, and the real InputController (offscreen platform)
add_executable (testWebViewHome
    "test_webview_home.cpp"
    "../../resources/qrc/main.qrc"
    # the spinners of the status bar and the entity screen title (loader_small.png)
    "../../resources/qrc/images.qrc"
    "../../src/webview/webviewsite.cpp"
    "../../src/ui/entity/webview.cpp"
    "../../src/ui/entity/webviewButtons.cpp"
    "../../src/ui/entity/entity.cpp"
    "../../src/ui/inputController.cpp"
    "../../src/ui/colors.cpp"
    # header only: contains the Q_OBJECT / Q_GADGET classes moc has to process
    "../../src/ui/fonts.h"
    "../../src/core/enums.h"
    "../../src/hardware/hardwareModel.h"
    "../../src/util.cpp"
    "../../src/logging.cpp"
    "../../3rd-party/QR-Code-generator/cpp/qrcodegen.cpp"
    )

add_test(NAME testWebViewHome
         COMMAND testWebViewHome)

target_link_libraries(testWebViewHome
    PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Qml
    Qt5::Quick
    Qt5::Test
    )

# Settings > Web view: the page as the settings menu loads it, with the real web view controller on a core that is not
# connected, the test page it opens, and the real InputController (offscreen platform)
add_executable (testWebViewSettings
    "test_webview_settings.cpp"
    "test_webview_settings.qrc"
    "../../resources/qrc/main.qrc"
    "../../resources/qrc/images.qrc"
    "../../src/webview/webviewitem.cpp"
    "../../src/webview/devicestatesource.cpp"
    "../../src/webview/webviewlibrary.cpp"
    "../../src/webview/webviewinput.cpp"
    "../../src/webview/webviewframequeue.cpp"
    "../../src/webview/webviewpinch.cpp"
    "../../src/webview/webviewrequest.cpp"
    "../../src/webview/webviewsite.cpp"
    "../../src/webview/webviewtexture.cpp"
    "../../src/ui/entity/webviewController.cpp"
    "../../src/ui/entity/entityController.cpp"
    "../../src/ui/entity/entity.cpp"
    "../../src/ui/entity/activity.cpp"
    "../../src/ui/entity/availableEntities.cpp"
    "../../src/ui/entity/button.cpp"
    "../../src/ui/entity/climate.cpp"
    "../../src/ui/entity/commandRetryPolicy.cpp"
    "../../src/ui/entity/configuredEntities.cpp"
    "../../src/ui/entity/cover.cpp"
    "../../src/ui/entity/light.cpp"
    "../../src/ui/entity/macro.cpp"
    "../../src/ui/entity/mediaPlayer.cpp"
    "../../src/ui/entity/number.cpp"
    "../../src/ui/entity/remote.cpp"
    "../../src/ui/entity/select.cpp"
    "../../src/ui/entity/sensor.cpp"
    "../../src/ui/entity/sequenceReadinessReport.cpp"
    "../../src/ui/entity/switch.cpp"
    "../../src/ui/entity/valueFormat.cpp"
    "../../src/ui/entity/voiceAssistant.cpp"
    "../../src/ui/entity/voiceSession.cpp"
    "../../src/ui/entity/webview.cpp"
    "../../src/ui/entity/webviewButtons.cpp"
    "../../src/ui/entity/entities.cpp"
    "../../src/ui/entity/entitiesProxy.cpp"
    "../../src/ui/entity/entityCommandPolicy.cpp"
    "../../src/ui/entity/entityScreens.cpp"
    # header only: contains the Q_GADGET class moc has to process
    "../../src/ui/entity/sequenceStep.h"
    "../../src/ui/imageDecoder.cpp"
    "../../src/ui/mediaImageProvider.cpp"
    "../../src/ui/notification.cpp"
    "../../src/ui/inputController.cpp"
    "../../src/ui/colors.cpp"
    # the icons of the rows: the resources with the embedded icon font
    "../../src/ui/resources.cpp"
    "../../src/ui/iconFont.cpp"
    "../../resources/qrc/icons.qrc"
    # header only: contains the Q_OBJECT / Q_GADGET classes moc has to process
    "../../src/ui/fonts.h"
    "../../src/hardware/hardwareModel.h"
    "../../src/config/config.cpp"
    "../../src/translation/translation.cpp"
    "../../src/core/core.cpp"
    # header only: contains the Q_GADGET enums moc has to process
    "../../src/core/enums.h"
    "../../src/util.cpp"
    "../../src/logging.cpp"
    "../../3rd-party/QR-Code-generator/cpp/qrcodegen.cpp"
    )

add_test(NAME testWebViewSettings
         COMMAND testWebViewSettings)

target_link_libraries(testWebViewSettings
    PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::GuiPrivate
    Qt5::Qml
    Qt5::Quick
    Qt5::WebSockets
    Qt5::Test
    )
