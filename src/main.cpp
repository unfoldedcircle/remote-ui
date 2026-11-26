// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QGuiApplication>
#include <QMetaObject>
#include <QQmlApplicationEngine>
#include <QQmlProperty>
#include <QScreen>
#include <QQuickWindow>
#include <QTimer>

#include <memory>

#include "config/config.h"
#include "core/core.h"
#include "dock/dockController.h"
#include "hardware/hardwareController.h"
#include "hardware/hardwareModel.h"
#include "integration/integrationController.h"
#include "logging.h"
#include "softwareupdate/softwareUpdate.h"
#include "translation/translation.h"
#include "ui/uiController.h"
#include "voice.h"

#ifdef UC_WEBVIEW
#include "system/battery.h"
#include "system/power.h"
#include "webview/devicestatesource.h"

// The device state for web pages follows the power mode and the battery (docs/webview.md, "Device state for web
// pages"). Power::resumedFromSuspend() comes before the mode change it brings.
static void connectDeviceState(uc::webview::DeviceStateSource *state, uc::hw::Power *power, uc::hw::Battery *battery) {
    using Source = uc::webview::DeviceStateSource;
    using Power = uc::hw::Power;
    static_assert(static_cast<int>(Source::Normal) == static_cast<int>(Power::Normal) &&
                      static_cast<int>(Source::Idle) == static_cast<int>(Power::Idle) &&
                      static_cast<int>(Source::LowPower) == static_cast<int>(Power::Low_power) &&
                      static_cast<int>(Source::Suspend) == static_cast<int>(Power::Suspend),
                  "DeviceStateSource::PowerMode has the values of Power::PowerMode");

    const auto updateBattery = [state, battery] { state->setBattery(battery->getLevel(), battery->getPowerSupply()); };
    state->setPowerMode(static_cast<Source::PowerMode>(power->getPowerMode()));
    updateBattery();
    QObject::connect(power, &Power::powerModeChanged, state, [state](Power::PowerMode, Power::PowerMode toPowerMode) {
        state->setPowerMode(static_cast<Source::PowerMode>(toPowerMode));
    });
    QObject::connect(power, &Power::resumedFromSuspend, state, &Source::resumedFromSuspend);
    QObject::connect(battery, &uc::hw::Battery::levelChanged, state, updateBattery);
    QObject::connect(battery, &uc::hw::Battery::powerSupplyChanged, state, updateBattery);
}
#endif

#ifdef Q_OS_UNIX
#include <sys/socket.h>
#include <unistd.h>

#include <QSocketNotifier>
#include <csignal>

// A termination signal is handed to the main thread through a socket pair, the only thing a signal handler may
// safely do. QCoreApplication::quit() from the handler was not safe (it takes locks the interrupted thread may
// hold) and, worse, is a no-op before the event loop runs: a SIGTERM during the first seconds of the start-up was
// lost, the app never exited, systemd killed it after its timeout and the failure handler rebooted the device.
// The notifier reads the byte once the event loop runs, so a signal that arrives earlier still quits the app.
static int s_terminationSocket[2] = {-1, -1};

void sigHandler(int s) {
    // a second signal terminates right away
    std::signal(s, SIG_DFL);
    const char    signalNumber = static_cast<char>(s);
    const ssize_t written = ::write(s_terminationSocket[0], &signalNumber, 1);
    Q_UNUSED(written)
}
#endif

int main(int argc, char *argv[]) {
    bool ok;
    int  width, height;

    auto model = uc::hw::HardwareModel::fromString(qgetenv("UC_MODEL").toUpper(), &ok);

    if (!ok) {
        model = uc::hw::HardwareModel::DEV;
    }

    QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);

    qputenv("QML_DISABLE_DISTANCEFIELD", "1");

    // set text rendering to native
    QQuickWindow::setTextRenderType(QQuickWindow::TextRenderType::NativeTextRendering);

    if (model == uc::hw::HardwareModel::DEV) {
        QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
        QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

        // Scale factor of the simulator window. Default 0.5 on macOS, where the desktop is a 2x Retina display and
        // the 480x850 panel is rendered at half size, and 1 on Linux and Windows (regular 1x displays; the desktop
        // scaling is handled by the platform). UC_DISPLAY_SCALE overrides it.
#ifdef Q_OS_MACOS
        const double defaultScale = 0.5;
#else
        const double defaultScale = 1.0;
#endif
        double ratio = qgetenv("UC_DISPLAY_SCALE").toDouble();
        if (ratio <= 0.0) {
            ratio = defaultScale;
        }
        qputenv("QT_SCALE_FACTOR", QString::number(ratio).toLocal8Bit());
    }

    qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));
    qputenv("QT_VIRTUALKEYBOARD_LAYOUT_PATH", "qrc:/keyboard/layouts");
    qputenv("QT_VIRTUALKEYBOARD_STYLE", "remotestyle");

    QGuiApplication app(argc, argv);
    // Heap allocated to control the destruction order: the engine must go before the controllers, see the end of main().
    auto engine = std::make_unique<QQmlApplicationEngine>();

#ifdef Q_OS_UNIX
    // At least SIGTERM is required to run on the device for proper systemd integration,
    // otherwise the recovery handler might get called when stopping the app!
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, s_terminationSocket) != 0) {
        qCCritical(lcApp()) << "Cannot create the termination socket pair, the app cannot be stopped by a signal";
    }
    QSocketNotifier terminationNotifier(s_terminationSocket[1], QSocketNotifier::Read, &app);
    QObject::connect(&terminationNotifier, &QSocketNotifier::activated, &app, [&app] {
        char signalNumber = 0;
        if (::read(s_terminationSocket[1], &signalNumber, 1) == 1) {
            qCInfo(lcApp()) << "Termination signal" << static_cast<int>(signalNumber) << "received, quitting";
        }
        app.quit();
    });
    std::signal(SIGINT, sigHandler);
    std::signal(SIGQUIT, sigHandler);
    std::signal(SIGTERM, sigHandler);
#endif

    engine->addImportPath("qrc:/keyboard");

    QCoreApplication::setOrganizationName("Unfolded Circle");
    QCoreApplication::setOrganizationDomain("uc.io");
    QCoreApplication::setApplicationName("remote-ui");

    QScreen *screen = QGuiApplication::primaryScreen();
    qCDebug(lcApp()) << "Screen Width" << screen->geometry().width();
    qCDebug(lcApp()) << "Screen Height" << screen->geometry().height();
    qCDebug(lcApp()) << "Orientation" << screen->orientation();

    if (model == uc::hw::HardwareModel::DEV) {
        width = qEnvironmentVariableIntValue("UC_DISPLAY_WIDTH") != 0 ? qEnvironmentVariableIntValue("UC_DISPLAY_WIDTH")
                                                                      : 480;
        height = qEnvironmentVariableIntValue("UC_DISPLAY_HEIGHT") != 0
                     ? qEnvironmentVariableIntValue("UC_DISPLAY_HEIGHT")
                     : 850;
    } else {
        width = screen->geometry().width();
        height = screen->geometry().height();
    }

    QString socketUrl = qgetenv("UC_SOCKET_URL");

    if (socketUrl.isEmpty()) {
        socketUrl = "ws://127.0.0.1:8080/ws";
    }

    uc::core::Api                          core(socketUrl, &app);
    uc::Config                             config(&core, &app);
    uc::SoftwareUpdate                     softwareUpdate(&core, &app);
    uc::hw::Controller                     hwController(model, &core, &config, &app);
#ifdef UC_WEBVIEW
    uc::webview::DeviceStateSource deviceState(&app);
    connectDeviceState(&deviceState, hwController.getPower(), hwController.getBattery());
#endif
    uc::ui::Controller                     uiController(model, width, height, engine.get(), &config, &core, &app);
    uc::integration::IntegrationController integrationController(&core, config.getLanguage(), &app);
    uc::dock::DockController               dockController(&core, &app);
    uc::ui::Translation                    translation(engine.get(), &core, &app);
    uc::Voice                              voice(&core, &app);

    QObject::connect(&integrationController, &uc::integration::IntegrationController::integrationIsConnecting,
                     &uiController, &uc::ui::Controller::onIntegrationIsConnecting);
    QObject::connect(&integrationController, &uc::integration::IntegrationController::integrationError, &uiController,
                     &uc::ui::Controller::onIntegrationError);
    QObject::connect(&integrationController, &uc::integration::IntegrationController::integrationDeleted, &uiController,
                     &uc::ui::Controller::onIntegrationDeleted);

    QObject::connect(&config, &uc::Config::languageChanged, &softwareUpdate, &uc::SoftwareUpdate::onLanguageChanged);
    QObject::connect(&config, &uc::Config::languageChanged, &translation, &uc::ui::Translation::onLanguageChanged);
    QObject::connect(&config, &uc::Config::languageChanged, &integrationController,
                     &uc::integration::IntegrationController::onLanguageChanged);

    const QUrl url(QStringLiteral("qrc:/main.qml"));

    QObject::connect(
        engine.get(), &QQmlApplicationEngine::objectCreated, &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl) {
                QCoreApplication::exit(-1);
            }
        },
        Qt::QueuedConnection);

    engine->load(url);

#ifdef Q_OS_MACOS
    if (model == uc::hw::HardwareModel::DEV) {
        // Qt 5.15 on macOS creates a window at its requested size times QT_SCALE_FACTOR: QCocoaWindow::initialize()
        // passes the native geometry back to QWindow::setGeometry() as if it were device independent. main.qml
        // compensates and requests ui.width * ui.ratio, but Cocoa keeps the minimum and maximum size at twice the size
        // the window got. macOS enforces them when it rearranges the windows after a display change, such as plugging
        // in or out a monitor, and the windows grow to twice their content. Fixing the limits to the size the windows
        // got prevents that. QQmlProperty::write() replaces the QML bindings of the limits, which the
        // QQmlEngine::retranslate() of a language change would otherwise evaluate again. Queued: the button simulator
        // is created in a call queued while loading main.qml.
        QTimer::singleShot(0, &app, [] {
            for (QWindow *window : QGuiApplication::topLevelWindows()) {
                if (window->handle() && window->minimumSize() == window->maximumSize()) {
                    QQmlProperty::write(window, "minimumWidth", window->width());
                    QQmlProperty::write(window, "minimumHeight", window->height());
                    QQmlProperty::write(window, "maximumWidth", window->width());
                    QQmlProperty::write(window, "maximumHeight", window->height());
                }
            }
        });
    }
#endif

    uiController.init();

    const int ret = app.exec();

    // Tear down the QML scene while the C++ side is still complete. The controllers above are destroyed in reverse
    // order of declaration, i.e. before an engine declared next to the application would be. They own the objects
    // behind the `ui`, `colors`, `fonts` and `resource` context properties and the QML singletons (Battery, Power,
    // Wifi, ...): destroying those first turns every reference in the still existing QML items into null and
    // re-evaluates hundreds of bindings, which floods the log with "TypeError: Cannot read property ... of null"
    // on every shutdown.
    engine.reset();

    return ret;
}
