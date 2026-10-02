project (testUi LANGUAGES CXX)

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

# the core response handler templates use generic lambdas
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Mostly for developing on macOS with an M1 or newer CPU. Old Qt version isn't available as arm64
set(CMAKE_OSX_ARCHITECTURES x86_64)

enable_testing(true)

add_executable (testCoverEntity
    "test_cover_entity.cpp"
    "../../src/ui/entity/cover.cpp"
    "../../src/ui/entity/entityScreens.cpp"
    "../../src/ui/entity/entity.cpp"
    "../../src/util.cpp"
    "../../src/logging.cpp"
    "../../3rd-party/QR-Code-generator/cpp/qrcodegen.cpp"
    )

add_test(NAME testCoverEntity
         COMMAND testCoverEntity)

target_link_libraries(testCoverEntity
    PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Qml
    Qt5::Test
    )

add_executable (testUiModels
    "test_models.cpp"
    "../../src/ui/page/pages.cpp"
    "../../src/ui/page/page.cpp"
    "../../src/ui/page/pageItem.cpp"
    "../../src/ui/profile/profiles.cpp"
    # header only: contains the Q_OBJECT class moc has to process
    "../../src/ui/profile/profile.h"
    "../../src/ui/group/group.cpp"
    "../../src/ui/group/groupItem.cpp"
    "../../src/ui/notification.cpp"
    "../../src/core/core.cpp"
    # header only: contains the Q_GADGET enums moc has to process
    "../../src/core/enums.h"
    "../../src/util.cpp"
    "../../src/logging.cpp"
    "../../3rd-party/QR-Code-generator/cpp/qrcodegen.cpp"
    )

add_test(NAME testUiModels
         COMMAND testUiModels)

# The icon fallback depends on what the embedded icon font can draw, so the test compiles the
# icon resources in (font + mapping + fallback mapping).
add_executable (testIconFont
    "test_icon_font.cpp"
    "../../src/ui/iconFont.cpp"
    "../../src/ui/resources.cpp"
    "../../src/util.cpp"
    "../../src/logging.cpp"
    # util.cpp generates QR codes
    "../../3rd-party/QR-Code-generator/cpp/qrcodegen.cpp"
    "../../resources/qrc/icons.qrc"
    )

add_test(NAME testIconFont
         COMMAND testIconFont)

target_link_libraries(testIconFont
    PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Qml
    Qt5::Test
    )

target_link_libraries(testUiModels
    PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Qml
    Qt5::WebSockets
    Qt5::Test
    )

add_executable (testCommandRetryPolicy
    "test_command_retry_policy.cpp"
    "../../src/ui/entity/commandRetryPolicy.cpp"
    )

add_test(NAME testCommandRetryPolicy
         COMMAND testCommandRetryPolicy)

target_link_libraries(testCommandRetryPolicy
    PUBLIC
    Qt5::Core
    Qt5::Test
    )

# the entity classes are QML items: uc::ui::entity::Light derives from QQuickPaintedItem
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Quick REQUIRED)

add_executable (testEntityController
    "test_entity_controller.cpp"
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
    "../../src/ui/entity/remote.cpp"
    "../../src/ui/entity/select.cpp"
    "../../src/ui/entity/sensor.cpp"
    "../../src/ui/entity/sequenceReadinessReport.cpp"
    "../../src/ui/entity/switch.cpp"
    "../../src/ui/entity/voiceAssistant.cpp"
    "../../src/ui/entity/voiceSession.cpp"
    "../../src/ui/entity/entities.cpp"
    "../../src/ui/entity/entitiesProxy.cpp"
    "../../src/ui/entity/entityCommandPolicy.cpp"
    "../../src/ui/entity/entityScreens.cpp"
    # header only: contains the Q_GADGET class moc has to process
    "../../src/ui/entity/sequenceStep.h"
    "../../src/ui/mediaImageProvider.cpp"
    "../../src/ui/notification.cpp"
    "../../src/config/config.cpp"
    "../../src/translation/translation.cpp"
    "../../src/core/core.cpp"
    # header only: contains the Q_GADGET enums moc has to process
    "../../src/core/enums.h"
    "../../src/util.cpp"
    "../../src/logging.cpp"
    "../../3rd-party/QR-Code-generator/cpp/qrcodegen.cpp"
    )

# uc::Config reports the UI version in its settings
target_compile_definitions(testEntityController PRIVATE APP_VERSION="test")

add_test(NAME testEntityController
         COMMAND testEntityController)

target_link_libraries(testEntityController
    PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Qml
    Qt5::Quick
    Qt5::WebSockets
    Qt5::Test
    )

add_executable (testUiEntities
    "test_entities.cpp"
    "../../src/ui/entity/entity.cpp"
    "../../src/ui/entity/select.cpp"
    "../../src/util.cpp"
    "../../src/logging.cpp"
    "../../3rd-party/QR-Code-generator/cpp/qrcodegen.cpp"
    )

add_test(NAME testUiEntities
         COMMAND testUiEntities)

target_link_libraries(testUiEntities
    PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Qml
    Qt5::Test
    )

add_executable (testEntityCommandPolicy
    "test_entity_command_policy.cpp"
    "../../src/ui/entity/entityCommandPolicy.cpp"
    )

add_test(NAME testEntityCommandPolicy
         COMMAND testEntityCommandPolicy)

target_link_libraries(testEntityCommandPolicy
    PUBLIC
    Qt5::Core
    Qt5::Test
    )

add_executable (testEntitiesStaleResponse
    "test_entities_stale_response.cpp"
    "../../src/ui/entity/entities.cpp"
    "../../src/ui/entity/entity.cpp"
    "../../src/ui/notification.cpp"
    "../../src/core/core.cpp"
    # header only: contains the Q_GADGET enums moc has to process
    "../../src/core/enums.h"
    "../../src/util.cpp"
    "../../src/logging.cpp"
    "../../3rd-party/QR-Code-generator/cpp/qrcodegen.cpp"
    )

add_test(NAME testEntitiesStaleResponse
         COMMAND testEntitiesStaleResponse)

target_link_libraries(testEntitiesStaleResponse
    PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Qml
    Qt5::WebSockets
    Qt5::Test
    )

add_executable (testVoiceSession
    "test_voice_session.cpp"
    "../../src/ui/entity/voiceSession.cpp"
    )

add_test(NAME testVoiceSession
         COMMAND testVoiceSession)

target_link_libraries(testVoiceSession
    PUBLIC
    Qt5::Core
    Qt5::Test
    )

# Checks the entity detail screen registry against the device class enums of the entity classes and against
# resources/qrc/main.qrc, so that a screen missing from the qrc fails here and not on the device. The entity
# sources are only compiled for the moc output of their device class enums.
add_executable (testEntityScreens
    "test_entity_screens.cpp"
    "../../src/ui/entity/entityScreens.cpp"
    "../../src/ui/entity/entity.cpp"
    "../../src/ui/entity/activity.cpp"
    "../../src/ui/entity/button.cpp"
    "../../src/ui/entity/climate.cpp"
    "../../src/ui/entity/cover.cpp"
    "../../src/ui/entity/light.cpp"
    "../../src/ui/entity/macro.cpp"
    "../../src/ui/entity/mediaPlayer.cpp"
    "../../src/ui/entity/remote.cpp"
    "../../src/ui/entity/select.cpp"
    "../../src/ui/entity/sensor.cpp"
    "../../src/ui/entity/switch.cpp"
    "../../src/ui/entity/voiceAssistant.cpp"
    "../../src/ui/entity/voiceSession.cpp"
    # header only: contains the Q_OBJECT class moc has to process
    "../../src/ui/entity/sequenceStep.h"
    "../../src/ui/mediaImageProvider.cpp"
    "../../src/util.cpp"
    "../../src/logging.cpp"
    "../../3rd-party/QR-Code-generator/cpp/qrcodegen.cpp"
    )

target_compile_definitions(testEntityScreens PRIVATE
    MAIN_QRC="${CMAKE_CURRENT_SOURCE_DIR}/../../resources/qrc/main.qrc"
    ENTITIES_QML_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../src/qml/components/entities"
    )

add_test(NAME testEntityScreens
         COMMAND testEntityScreens)

target_link_libraries(testEntityScreens
    PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Qml
    Qt5::Quick
    Qt5::WebSockets
    Qt5::Test
    )

add_executable (testGroupController
    "test_group_controller.cpp"
    "../../src/ui/group/groupController.cpp"
    "../../src/ui/group/group.cpp"
    "../../src/ui/group/groupItem.cpp"
    "../../src/ui/notification.cpp"
    "../../src/core/core.cpp"
    # header only: contains the Q_GADGET enums moc has to process
    "../../src/core/enums.h"
    "../../src/util.cpp"
    "../../src/logging.cpp"
    "../../3rd-party/QR-Code-generator/cpp/qrcodegen.cpp"
    )

add_test(NAME testGroupController
         COMMAND testGroupController)

target_link_libraries(testGroupController
    PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Qml
    Qt5::WebSockets
    Qt5::Test
    )

add_executable (testResources
    "test_resources.cpp"
    "../../src/ui/iconFont.cpp"
    "../../src/ui/resources.cpp"
    "../../src/util.cpp"
    "../../src/logging.cpp"
    # util.cpp generates QR codes
    "../../3rd-party/QR-Code-generator/cpp/qrcodegen.cpp"
    )

add_test(NAME testResources
         COMMAND testResources)

target_link_libraries(testResources
    PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Qml
    Qt5::Test
    )

add_executable (testDockModels
    "test_dock_models.cpp"
    "../../src/dock/configuredDocks.cpp"
    "../../src/dock/discoveredDocks.cpp"
    "../../src/logging.cpp"
    )

add_test(NAME testDockModels
         COMMAND testDockModels)

target_link_libraries(testDockModels
    PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Qml
    Qt5::Test
    )

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Network REQUIRED)

add_executable (testVoice
    "test_voice.cpp"
    "../../src/voice.cpp"
    "../../src/core/core.cpp"
    # header only: contains the Q_GADGET enums moc has to process
    "../../src/core/enums.h"
    "../../src/ui/notification.cpp"
    "../../src/util.cpp"
    "../../src/logging.cpp"
    "../../3rd-party/QR-Code-generator/cpp/qrcodegen.cpp"
    )

add_test(NAME testVoice
         COMMAND testVoice)

target_link_libraries(testVoice
    PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Qml
    Qt5::Network
    Qt5::WebSockets
    Qt5::Test
    )
