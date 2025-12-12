// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QGuiApplication>
#include <QMetaObject>
#include <QQmlApplicationEngine>
#include <QScreen>
#include <QQuickWindow>

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

#ifdef Q_OS_UNIX
#include <csignal>

// a minimal signal handler for the embedded device
void sigHandler(int s) {
    std::signal(s, SIG_DFL);
    qApp->quit();
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

        double ratio = qgetenv("UC_DISPLAY_SCALE").toDouble() != 0.0 ? qgetenv("UC_DISPLAY_SCALE").toDouble() : 0.5;
        qputenv("QT_SCALE_FACTOR", QString::number(ratio).toLocal8Bit());
    }

    qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));
    qputenv("QT_VIRTUALKEYBOARD_LAYOUT_PATH", "qrc:/keyboard/layouts");
    qputenv("QT_VIRTUALKEYBOARD_STYLE", "remotestyle");

    QGuiApplication       app(argc, argv);
    QQmlApplicationEngine engine;

#ifdef Q_OS_UNIX
    // At least SIGTERM is required to run on the device for proper systemd integration,
    // otherwise the recovery handler might get called when stopping the app!
    std::signal(SIGINT, sigHandler);
    std::signal(SIGQUIT, sigHandler);
    std::signal(SIGTERM, sigHandler);
#endif

    engine.addImportPath("qrc:/keyboard");

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
    uc::ui::Controller                     uiController(model, width, height, &engine, &config, &core, &app);
    uc::integration::IntegrationController integrationController(&core, config.getLanguage(), &app);
    uc::dock::DockController               dockController(&core, &app);
    uc::ui::Translation                    translation(&engine, &core, &app);
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
        &engine, &QQmlApplicationEngine::objectCreated, &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl) {
                QCoreApplication::exit(-1);
            }
        },
        Qt::QueuedConnection);

    engine.load(url);

    uiController.init();

    return app.exec();
}
