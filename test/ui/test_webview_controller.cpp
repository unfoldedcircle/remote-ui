// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QJsonDocument>
#include <QPointer>
#include <QTemporaryFile>
#include <QWebSocket>
#include <QWebSocketServer>
#include <QtTest>

#include <functional>
#include <utility>

#include "config/config.h"
#include "core/core.h"
#include "hardware/hardwareModel.h"
#include "ui/entity/entityController.h"
#include "ui/entity/webview.h"
#include "ui/entity/webviewButtons.h"
#include "ui/entity/webviewController.h"
#include "ui/notification.h"

using uc::core::UiEntityCommand;
using uc::hw::HardwareModel;
using uc::ui::EntityController;
using uc::ui::WebViewController;
using uc::ui::entity::Webview;
using uc::ui::entity::WebviewButtons;

// an unreachable address: the socket connection attempt is asynchronous and never completes during a test
static const QString kTestUrl = QStringLiteral("ws://127.0.0.1:1/ws");
static const QString kWebviewId = QStringLiteral("uc_web.main.1");

/**
 * A core on a local WebSocket for the declaration and the global web view settings: asks the UI to authenticate,
 * accepts it, records the entity types of every set_ui_info request, answers get_webview_cfg with its settings and
 * set_webview_cfg with the changed settings followed by a configuration_change event, or refuses it with
 * `refuseCode`.
 */
class FakeCore {
 public:
    FakeCore() {
        QObject::connect(&m_server, &QWebSocketServer::newConnection, [this] {
            QWebSocket *socket = m_server.nextPendingConnection();
            m_socket = socket;
            QObject::connect(socket, &QWebSocket::textMessageReceived,
                             [this, socket](const QString &message) { onMessage(socket, message); });
            socket->sendTextMessage(QStringLiteral(R"({"kind": "event", "msg": "auth_required", "msg_data": {}})"));
        });
        m_server.listen(QHostAddress::LocalHost);
    }

    QString url() const { return QStringLiteral("ws://127.0.0.1:%1/ws").arg(m_server.serverPort()); }

    // a change of the settings made elsewhere, e.g. in the web configurator: the configuration_change event
    void changeSettings(const QVariantMap &update) {
        for (auto it = update.constBegin(); it != update.constEnd(); ++it) {
            settings.insert(it.key(), it.value());
        }
        sendJson(QVariantMap{{"kind", "event"},
                             {"msg", "configuration_change"},
                             {"msg_data", QVariantMap{{"new_state", QVariantMap{{"webview", settings}}}}}});
    }

    // the entity types of each set_ui_info, in order
    QList<QStringList> declarations;
    // the core's global web view settings
    QVariantMap settings{{"allow_invalid_certificates", false}, {"color_scheme", "dark"}, {"website_data", "keep"}};
    // the msg_data of each set_webview_cfg, in order
    QList<QVariantMap> settingsUpdates;
    // set_webview_cfg is refused with this code (403: the session lacks the admin scope), 0: accepted
    int refuseCode = 0;

 private:
    void sendJson(const QVariantMap &map) {
        if (m_socket) {
            m_socket->sendTextMessage(QString::fromUtf8(QJsonDocument::fromVariant(map).toJson()));
        }
    }

    void onMessage(QWebSocket *socket, const QString &message) {
        const QVariantMap request = QJsonDocument::fromJson(message.toUtf8()).toVariant().toMap();
        const QString     msg = request.value("msg").toString();
        const int         id = request.value("id").toInt();
        if (msg == QLatin1String("auth")) {
            socket->sendTextMessage(
                QStringLiteral(R"({"kind": "resp", "req_id": %1, "code": 200, "msg": "authentication"})").arg(id));
        } else if (msg == QLatin1String("set_ui_info")) {
            declarations.append(request.value("msg_data").toMap().value("entity_types").toStringList());
            socket->sendTextMessage(
                QStringLiteral(R"({"kind": "resp", "req_id": %1, "code": 200, "msg": "result"})").arg(id));
        } else if (msg == QLatin1String("get_webview_cfg")) {
            sendJson(QVariantMap{
                {"kind", "resp"}, {"req_id", id}, {"code", 200}, {"msg", "webview_cfg"}, {"msg_data", settings}});
        } else if (msg == QLatin1String("set_webview_cfg")) {
            settingsUpdates.append(request.value("msg_data").toMap());
            if (refuseCode != 0) {
                sendJson(
                    QVariantMap{{"kind", "resp"},
                                {"req_id", id},
                                {"code", refuseCode},
                                {"msg", "result"},
                                {"msg_data", QVariantMap{{"code", "FORBIDDEN"}, {"message", "Admin scope required"}}}});
                return;
            }
            const QVariantMap update = settingsUpdates.last();
            for (auto it = update.constBegin(); it != update.constEnd(); ++it) {
                if (it.key() != QLatin1String("buttons")) {
                    settings.insert(it.key(), it.value());
                    continue;
                }
                // merged button by button, null resets a button to its default
                QVariantMap       buttons = settings.value("buttons").toMap();
                const QVariantMap changes = it.value().toMap();
                for (auto button = changes.constBegin(); button != changes.constEnd(); ++button) {
                    if (button.value().isNull()) {
                        buttons.remove(button.key());
                    } else {
                        buttons.insert(button.key(), button.value());
                    }
                }
                settings.insert("buttons", buttons);
            }
            sendJson(QVariantMap{
                {"kind", "resp"}, {"req_id", id}, {"code", 200}, {"msg", "webview_cfg"}, {"msg_data", settings}});
            changeSettings(QVariantMap());
        }
    }

    QWebSocketServer     m_server{QStringLiteral("core"), QWebSocketServer::NonSecureMode};
    QPointer<QWebSocket> m_socket;
};

/**
 * The UI executes the commands of web view entities, which the core routes to it (core-api entity_webview.md
 * "Command routing"): the result codes of entity_command, what reaches QML, the declaration of the entity types with
 * set_ui_info, the website data setting, and the secret options of a web view that an event does not carry.
 */
class testWebviewController : public QObject {
    Q_OBJECT

 private slots:
    void initTestCase();
    void commands_ofAnotherEntityTypeAreRefused();
    void commands_unknownCommandIsRefused();
    void commands_unknownWebviewIsNotFound();
    void commands_refusedWithoutTheWebView();
    void open_isPassedToQml();
    void open_isRefusedDuringTheOnboarding();
    void reload_isPassedToQml();
    void clear_isNotImplementedWithoutLibrarySupport();
    void clear_answersWhenTheLibraryIsDoneAndReloadsEveryShownPage();
    void clear_failedOrRefusedIsAnInternalError();
    void websiteData_isClearedWhenAViewCloses();
    void declaration_namesWebviewsAfterTheAuthenticationWhenTheLibraryIsInstalled();
    void declaration_withoutTheLibraryNamesNoWebviews();
    void declaration_isWithdrawnWhenTheLibraryFailsToLoadAtACommand();
    void declaration_isWithdrawnWhenAPageFindsTheLibraryUnusable();
    void declaration_namesALibraryThatAPageLoaded();
    void fullscreen_isRecorded();
    void settings_areTheCoresAfterTheAuthentication();
    void settings_aChangeSendsOnlyThatSettingAndFollowsTheAnswer();
    void settings_aRefusedChangeKeepsTheSettingAndTellsWhy();
    void settings_unknownOrUnchangedValuesSendNothing();
    void settings_aChangeMadeElsewhereShowsAtOnce();
    void buttons_areTheModelsWithTheCoresActionOrTheDefault_data();
    void buttons_areTheModelsWithTheCoresActionOrTheDefault();
    void buttons_aChangeSendsOnlyThatButtonAndFollowsTheAnswer();
    void buttons_invalidOrUnchangedSendNothing();
    void clearFromTheSettings_isTheCommandsClear();
    void clearFromTheSettings_reportsAFailure();
    void clearFromTheSettings_isUnsupportedWithAnOlderLibrary();
    void optionsEventWithoutSecrets_isNotApplied();
    void optionsEventWithoutSecretOptions_isApplied();

 private:
    struct Fixture {
        using ClearStart = WebViewController::ClearStart;

        // the notifications of a refused change
        uc::ui::Notification notification;
        uc::core::Api     api;
        EntityController  entities{&api, QStringLiteral("en"), uc::Config::UnitSystems::Metric, 0};
        WebViewController    controller;
        // the library: installed (a file check) and whether it loads at the first use (counted)
        bool              installed = true;
        bool              available = true;
        int               loads = 0;
        // the library behind clearing: what it says to a request (a library older than 0.8.0 by default), the
        // requests (all) and the reports still to come
        ClearStart                       clearStart = ClearStart::Unsupported;
        // whether the library can clear (canClearWebsiteData())
        bool                             clearSupported = false;
        QList<bool>                      clears;
        QList<std::function<void(bool)>> clearDone;
        // the answers to the commands: code and message, in order
        QList<int>  codes;
        QStringList messages;

        // the core: a FakeCore's url, or none; the remote model whose buttons the settings have
        explicit Fixture(const QString &url = kTestUrl, HardwareModel::Enum model = HardwareModel::UCR3)
            : api(url), controller(&api, &entities, QStringLiteral("1.2.3"), model) {
            controller.setInstalledForTest([this] { return installed; });
            controller.setAvailabilityForTest([this] {
                loads++;
                return available;
            });
            controller.setClearWebsiteDataForTest([this](bool all, std::function<void(bool)> done) {
                clears.append(all);
                if (clearStart == ClearStart::Started) {
                    clearDone.append(std::move(done));
                }
                return clearStart;
            });
            controller.setGlobalSettingsForTest([this](const QString &colorScheme, bool allowInvalidCertificates) {
                globalSettings.append(colorScheme +
                                      (allowInvalidCertificates ? QStringLiteral(" true") : QStringLiteral(" false")));
            });
            controller.setClearSupportedForTest([this] { return clearSupported; });
        }

        // the global settings passed to the library: "<colour scheme> <allow invalid certificates>"
        QStringList globalSettings;

        // executes a command: its code when it was answered at once, else 0 (the answer comes later, in `codes`)
        int run(const UiEntityCommand &command, QString *message = nullptr) {
            const int answered = codes.size();
            controller.execute(command, [this](int code, const QString &text) {
                codes.append(code);
                messages.append(text);
            });
            if (codes.size() == answered) {
                return 0;
            }
            if (message) {
                *message = messages.last();
            }
            return codes.last();
        }

        void addWebview(const QVariantMap &options = QVariantMap({{"url", "https://a.example/"}})) {
            uc::core::Entity entity;
            entity.id = kWebviewId;
            entity.type = QStringLiteral("Webview");
            entity.name = QVariantMap({{QStringLiteral("en"), QStringLiteral("Dashboard")}});
            entity.integrationId = QStringLiteral("uc_web.main");
            entity.options = options;
            entity.attributes = QVariantMap({{QStringLiteral("state"), QStringLiteral("ON")}});
            entities.onEntityAdded(entity);
        }

        Webview *webview() { return qobject_cast<Webview *>(entities.get(kWebviewId)); }
    };

    static UiEntityCommand command(const QString &cmdId, const QString &entityId = kWebviewId,
                                   const QString &entityType = QStringLiteral("webview")) {
        UiEntityCommand c;
        c.reqId = 42;
        c.entityId = entityId;
        c.entityType = entityType;
        c.cmdId = cmdId;
        return c;
    }

    static QVariantMap fromJson(const char *json) { return QJsonDocument::fromJson(json).toVariant().toMap(); }

    // the token the UI authenticates with at a FakeCore (UC_TOKEN_PATH)
    QTemporaryFile m_token;
};

void testWebviewController::initTestCase() {
    QVERIFY(m_token.open());
    m_token.write("token");
    m_token.flush();
    qputenv("UC_TOKEN_PATH", QFile::encodeName(m_token.fileName()));
}

void testWebviewController::commands_ofAnotherEntityTypeAreRefused() {
    Fixture f;
    f.addWebview();
    QString message;
    QCOMPARE(f.run(command(QStringLiteral("open"), kWebviewId, QStringLiteral("light")), &message), 400);
    QVERIFY(!message.isEmpty());
}

void testWebviewController::commands_unknownCommandIsRefused() {
    Fixture f;
    f.addWebview();
    QSignalSpy open(&f.controller, &WebViewController::openRequested);
    QString    message;
    QCOMPARE(f.run(command(QStringLiteral("print")), &message), 400);
    QCOMPARE(f.run(command(QString()), &message), 400);
    QCOMPARE(open.count(), 0);
}

void testWebviewController::commands_unknownWebviewIsNotFound() {
    Fixture f;
    f.addWebview();
    QString message;
    QCOMPARE(f.run(command(QStringLiteral("open"), QStringLiteral("uc_web.main.unknown")), &message), 404);
}

void testWebviewController::commands_refusedWithoutTheWebView() {
    Fixture f;
    f.addWebview();
    f.available = false;
    QSignalSpy open(&f.controller, &WebViewController::openRequested);
    QString    message;
    QCOMPARE(f.run(command(QStringLiteral("open")), &message), 503);
    QCOMPARE(open.count(), 0);
}

void testWebviewController::open_isPassedToQml() {
    Fixture f;
    f.addWebview();
    QSignalSpy open(&f.controller, &WebViewController::openRequested);
    QString    message;
    QCOMPARE(f.run(command(QStringLiteral("open")), &message), 200);
    QCOMPARE(open.count(), 1);
    QCOMPARE(open.first().first().toString(), kWebviewId);
}

void testWebviewController::open_isRefusedDuringTheOnboarding() {
    // the onboarding screens show no web view: `open` is answered 503, which the core passes on to its caller, and
    // nothing opens; the library is not loaded for it. The other commands work as always.
    Fixture f;
    f.addWebview();
    f.controller.setOnboarding(true);
    QSignalSpy open(&f.controller, &WebViewController::openRequested);
    QSignalSpy reload(&f.controller, &WebViewController::reloadRequested);
    QString    message;
    QCOMPARE(f.run(command(QStringLiteral("open")), &message), 503);
    QCOMPARE(message, QStringLiteral("The user interface can't show a web view during the onboarding of the Remote"));
    QCOMPARE(open.count(), 0);
    QCOMPARE(f.loads, 0);
    QCOMPARE(f.run(command(QStringLiteral("reload")), &message), 200);
    QCOMPARE(reload.count(), 1);

    // after the onboarding
    f.controller.setOnboarding(false);
    QCOMPARE(f.run(command(QStringLiteral("open")), &message), 200);
    QCOMPARE(open.count(), 1);
}

void testWebviewController::reload_isPassedToQml() {
    Fixture f;
    f.addWebview();
    QSignalSpy reload(&f.controller, &WebViewController::reloadRequested);
    QString    message;
    // accepted also when the web view is not shown: it has no effect then
    QCOMPARE(f.run(command(QStringLiteral("reload")), &message), 200);
    QCOMPARE(reload.count(), 1);
    QCOMPARE(reload.first().first().toString(), kWebviewId);
}

void testWebviewController::clear_isNotImplementedWithoutLibrarySupport() {
    // a library older than 0.8.0 has no uc_webview_global_clear_data()
    Fixture f;
    f.addWebview();
    QSignalSpy reload(&f.controller, &WebViewController::reloadRequested);
    QString    message;
    QCOMPARE(f.run(command(QStringLiteral("clear_cache")), &message), 501);
    QVERIFY(!message.isEmpty());
    QCOMPARE(f.run(command(QStringLiteral("clear_data")), &message), 501);
    QCOMPARE(f.clears, (QList<bool>{false, true}));
    QCOMPARE(reload.count(), 0);
}

void testWebviewController::clear_answersWhenTheLibraryIsDoneAndReloadsEveryShownPage() {
    Fixture f;
    f.addWebview();
    f.clearStart = WebViewController::ClearStart::Started;
    QSignalSpy reload(&f.controller, &WebViewController::reloadRequested);

    // accepted by the library: no answer and no reload until it reports
    QCOMPARE(f.run(command(QStringLiteral("clear_data"))), 0);
    QCOMPARE(f.run(command(QStringLiteral("clear_cache"))), 0);
    QCOMPARE(f.clears, (QList<bool>{true, false}));
    QVERIFY(f.codes.isEmpty());
    QCOMPARE(reload.count(), 0);

    f.clearDone.takeFirst()(true);
    QCOMPARE(f.codes, QList<int>{200});
    QCOMPARE(reload.count(), 1);
    // every web view shares the website data: every shown page reloads
    QVERIFY(reload.first().first().toString().isEmpty());
    f.clearDone.takeFirst()(true);
    QCOMPARE(f.codes, (QList<int>{200, 200}));
    QCOMPARE(reload.count(), 2);
}

void testWebviewController::clear_failedOrRefusedIsAnInternalError() {
    Fixture f;
    f.addWebview();
    QSignalSpy reload(&f.controller, &WebViewController::reloadRequested);

    // the library reports a failure: 500, nothing reloads
    f.clearStart = WebViewController::ClearStart::Started;
    QCOMPARE(f.run(command(QStringLiteral("clear_cache"))), 0);
    f.clearDone.takeFirst()(false);
    QCOMPARE(f.codes, QList<int>{500});
    QVERIFY(!f.messages.last().isEmpty());

    // the library refuses the request (its function returned -1): 500 at once
    f.clearStart = WebViewController::ClearStart::Refused;
    QString message;
    QCOMPARE(f.run(command(QStringLiteral("clear_data")), &message), 500);
    QVERIFY(!message.isEmpty());
    QVERIFY(f.clearDone.isEmpty());
    QCOMPARE(reload.count(), 0);
}

void testWebviewController::websiteData_isClearedWhenAViewCloses() {
    Fixture f;
    // keep: nothing
    f.controller.viewClosed();
    QVERIFY(f.clears.isEmpty());

    emit f.api.cfgWebviewChanged(
        uc::core::Api::parseWebviewCfg(fromJson(R"({"website_data": "clear_cache_on_close"})")));
    f.controller.viewClosed();
    emit f.api.cfgWebviewChanged(uc::core::Api::parseWebviewCfg(fromJson(R"({"website_data": "clear_all_on_close"})")));
    f.controller.viewClosed();
    QCOMPARE(f.clears, (QList<bool>{false, true}));
    QCOMPARE(f.controller.webviewCfg().websiteData, QStringLiteral("clear_all_on_close"));

    // with library support: cleared, no command to answer and no page to reload (the view is closed)
    QSignalSpy reload(&f.controller, &WebViewController::reloadRequested);
    f.clearStart = WebViewController::ClearStart::Started;
    f.controller.viewClosed();
    QCOMPARE(f.clears.size(), 3);
    f.clearDone.takeFirst()(true);
    QCOMPARE(reload.count(), 0);
    QVERIFY(f.codes.isEmpty());
}

void testWebviewController::declaration_namesWebviewsAfterTheAuthenticationWhenTheLibraryIsInstalled() {
    FakeCore core;
    Fixture  f(core.url());

    // at once, before any web view entity, and without loading the library: the core reports web views ON from the
    // start
    QTRY_COMPARE(core.declarations.size(), 1);
    QCOMPARE(core.declarations.first(), QStringList{QStringLiteral("webview")});
    QVERIFY(f.controller.declaresWebviews());
    QCOMPARE(f.loads, 0);

    // the first command loads it
    f.addWebview();
    QCOMPARE(f.loads, 0);
    QCOMPARE(f.run(command(QStringLiteral("open"))), 200);
    QCOMPARE(f.loads, 1);
    QCOMPARE(f.controller.entityTypesToDeclare(), QStringList{QStringLiteral("webview")});
}

void testWebviewController::declaration_withoutTheLibraryNamesNoWebviews() {
    FakeCore core;
    Fixture  f(core.url());
    f.installed = false;
    f.available = false;

    // the declaration names no entity type: the core keeps web views UNAVAILABLE
    QTRY_COMPARE(core.declarations.size(), 1);
    QVERIFY(core.declarations.first().isEmpty());
    QVERIFY(!f.controller.declaresWebviews());
    f.addWebview();
    QCOMPARE(f.loads, 0);
}

void testWebviewController::declaration_isWithdrawnWhenTheLibraryFailsToLoadAtACommand() {
    FakeCore core;
    Fixture  f(core.url());
    f.available = false;
    QTRY_COMPARE(core.declarations.size(), 1);
    QCOMPARE(core.declarations.first(), QStringList{QStringLiteral("webview")});

    // installed, but it does not load: the command is refused and the declaration withdrawn
    f.addWebview();
    QSignalSpy open(&f.controller, &WebViewController::openRequested);
    QString    message;
    QCOMPARE(f.run(command(QStringLiteral("open")), &message), 503);
    QVERIFY(!message.isEmpty());
    QCOMPARE(open.count(), 0);
    QTRY_COMPARE(core.declarations.size(), 2);
    QVERIFY(core.declarations.last().isEmpty());
    QVERIFY(!f.controller.declaresWebviews());

    // for the rest of the process, although the file is still there: the next authentication declares nothing either
    QCOMPARE(f.run(command(QStringLiteral("reload"))), 503);
    QVERIFY(f.controller.entityTypesToDeclare().isEmpty());
}

void testWebviewController::declaration_isWithdrawnWhenAPageFindsTheLibraryUnusable() {
    FakeCore core;
    Fixture  f(core.url());
    QTRY_COMPARE(core.declarations.size(), 1);

    // the library reports its load by a page: loaded, the declaration stays as it is
    f.controller.libraryLoaded(true);
    QVERIFY(f.controller.declaresWebviews());

    // then its initialisation failed: withdrawn, once
    f.controller.libraryLoaded(false);
    f.controller.libraryLoaded(false);
    QTRY_COMPARE(core.declarations.size(), 2);
    QVERIFY(core.declarations.last().isEmpty());
    QVERIFY(!f.controller.declaresWebviews());
}

void testWebviewController::declaration_namesALibraryThatAPageLoaded() {
    // a library the installation check does not find, which the dynamic linker found for a page
    FakeCore core;
    Fixture  f(core.url());
    f.installed = false;
    QTRY_COMPARE(core.declarations.size(), 1);
    QVERIFY(core.declarations.first().isEmpty());

    f.controller.libraryLoaded(true);
    QTRY_COMPARE(core.declarations.size(), 2);
    QCOMPARE(core.declarations.last(), QStringList{QStringLiteral("webview")});
}

void testWebviewController::fullscreen_isRecorded() {
    Fixture    f;
    QSignalSpy changed(&f.controller, &WebViewController::fullscreenEntityIdChanged);
    f.controller.setFullscreenEntityId(kWebviewId);
    f.controller.setFullscreenEntityId(kWebviewId);
    QCOMPARE(changed.count(), 1);
    f.controller.setFullscreenEntityId(QString());
    QCOMPARE(changed.count(), 2);
}

void testWebviewController::settings_areTheCoresAfterTheAuthentication() {
    FakeCore core;
    core.settings = QVariantMap{
        {"allow_invalid_certificates", true}, {"color_scheme", "light"}, {"website_data", "clear_cache_on_close"}};
    Fixture    f(core.url());
    QSignalSpy changed(&f.controller, &WebViewController::webviewCfgChanged);

    // the Core-API's defaults until the core answers get_webview_cfg
    QCOMPARE(f.controller.colorScheme(), QStringLiteral("dark"));
    QCOMPARE(f.controller.websiteData(), QStringLiteral("keep"));
    QVERIFY(!f.controller.allowInvalidCertificates());

    QTRY_COMPARE(f.controller.colorScheme(), QStringLiteral("light"));
    QCOMPARE(f.controller.websiteData(), QStringLiteral("clear_cache_on_close"));
    QVERIFY(f.controller.allowInvalidCertificates());
    QVERIFY(changed.count() >= 1);
    QCOMPARE(f.globalSettings.last(), QStringLiteral("light true"));
}

void testWebviewController::settings_aChangeSendsOnlyThatSettingAndFollowsTheAnswer() {
    FakeCore core;
    Fixture  f(core.url());
    QTRY_COMPARE(f.globalSettings.size(), 1);  // get_webview_cfg
    QSignalSpy changed(&f.controller, &WebViewController::webviewCfgChanged);
    QSignalSpy notifications(&f.notification, &uc::ui::Notification::notificationCreated);

    // the property follows the core's answer, not the write: a refused change never shows
    f.controller.setProperty("colorScheme", QStringLiteral("light"));
    QCOMPARE(f.controller.colorScheme(), QStringLiteral("dark"));
    QTRY_COMPARE(f.controller.colorScheme(), QStringLiteral("light"));
    QCOMPARE(core.settingsUpdates, (QList<QVariantMap>{QVariantMap{{"color_scheme", "light"}}}));
    QCOMPARE(f.globalSettings.last(), QStringLiteral("light false"));

    f.controller.setProperty("allowInvalidCertificates", true);
    QTRY_VERIFY(f.controller.allowInvalidCertificates());
    f.controller.setProperty("websiteData", QStringLiteral("clear_all_on_close"));
    QTRY_COMPARE(f.controller.websiteData(), QStringLiteral("clear_all_on_close"));
    QCOMPARE(core.settingsUpdates.size(), 3);
    QCOMPARE(core.settingsUpdates.at(1), (QVariantMap{{"allow_invalid_certificates", true}}));
    QCOMPARE(core.settingsUpdates.at(2), (QVariantMap{{"website_data", "clear_all_on_close"}}));
    QCOMPARE(f.globalSettings.last(), QStringLiteral("light true"));
    // the website data setting applies when a web view closes
    f.controller.viewClosed();
    QCOMPARE(f.clears, QList<bool>{true});
    // no notification: every change went through
    QCOMPARE(notifications.count(), 0);
    QVERIFY(changed.count() >= 3);
}

void testWebviewController::settings_aRefusedChangeKeepsTheSettingAndTellsWhy() {
    FakeCore core;
    core.refuseCode = 403;
    Fixture f(core.url());
    QTRY_COMPARE(f.globalSettings.size(), 1);  // get_webview_cfg
    QSignalSpy changed(&f.controller, &WebViewController::webviewCfgChanged);
    QSignalSpy notifications(&f.notification, &uc::ui::Notification::notificationCreated);

    // the switch has toggled already: the setting is reported again, as it is, so that it switches back
    f.controller.setProperty("allowInvalidCertificates", true);
    QTRY_COMPARE(notifications.count(), 1);
    QVERIFY(!f.controller.allowInvalidCertificates());
    QCOMPARE(changed.count(), 1);
    QVERIFY(notifications.first().at(0).toString().contains(QStringLiteral("Admin scope required")));
    QVERIFY(notifications.first().at(1).toBool());
    QCOMPARE(f.globalSettings.last(), QStringLiteral("dark false"));
}

void testWebviewController::settings_unknownOrUnchangedValuesSendNothing() {
    FakeCore core;
    Fixture  f(core.url());
    QTRY_COMPARE(core.declarations.size(), 1);
    QTRY_COMPARE(f.globalSettings.size(), 1);  // get_webview_cfg

    f.controller.setProperty("colorScheme", QStringLiteral("sepia"));
    f.controller.setProperty("websiteData", QStringLiteral("forget"));
    f.controller.setProperty("colorScheme", QStringLiteral("dark"));
    f.controller.setProperty("websiteData", QStringLiteral("keep"));
    f.controller.setProperty("allowInvalidCertificates", false);
    // a request that would be sent goes before an answer to a later one: wait for a round trip
    f.controller.setProperty("colorScheme", QStringLiteral("light"));
    QTRY_COMPARE(f.controller.colorScheme(), QStringLiteral("light"));
    QCOMPARE(core.settingsUpdates, (QList<QVariantMap>{QVariantMap{{"color_scheme", "light"}}}));
}

void testWebviewController::settings_aChangeMadeElsewhereShowsAtOnce() {
    FakeCore core;
    Fixture  f(core.url());
    QTRY_COMPARE(core.declarations.size(), 1);
    QTRY_COMPARE(f.globalSettings.size(), 1);  // get_webview_cfg
    QSignalSpy changed(&f.controller, &WebViewController::webviewCfgChanged);

    core.changeSettings(QVariantMap{{"color_scheme", "light"}, {"website_data", "clear_all_on_close"}});
    QTRY_COMPARE(changed.count(), 1);
    QCOMPARE(f.controller.colorScheme(), QStringLiteral("light"));
    QCOMPARE(f.controller.websiteData(), QStringLiteral("clear_all_on_close"));
    QCOMPARE(f.globalSettings.last(), QStringLiteral("light false"));
    QVERIFY(core.settingsUpdates.isEmpty());
}

void testWebviewController::buttons_areTheModelsWithTheCoresActionOrTheDefault_data() {
    QTest::addColumn<int>("model");
    QTest::addColumn<QStringList>("ownButtons");
    QTest::addColumn<QStringList>("otherButtons");

    QTest::newRow("Remote 3") << static_cast<int>(HardwareModel::UCR3) << QStringList{"STOP", "RECORD", "MENU"}
                              << QStringList{"RED", "GREEN", "YELLOW", "BLUE"};
    QTest::newRow("Remote Two") << static_cast<int>(HardwareModel::UCR2)
                                << QStringList{"RED", "GREEN", "YELLOW", "BLUE"}
                                << QStringList{"STOP", "RECORD", "MENU"};
}

void testWebviewController::buttons_areTheModelsWithTheCoresActionOrTheDefault() {
    // core-api "Button settings": the settings contain every button of the model with its configured or default
    // action; a button of the other model is stored by the core but not shown, an action the UI does not know is the
    // default (ADR 0018)
    QFETCH(int, model);
    QFETCH(QStringList, ownButtons);
    QFETCH(QStringList, otherButtons);
    FakeCore core;
    core.settings.insert("buttons", fromJson(R"({"VOLUME_UP": {"action": "pass"}, "VOLUME_DOWN": {"action": "pass"},
        "BACK": {"action": "function", "function": "teleport"}, "RED": {"action": "key", "key": "ColorF0Red"},
        "MENU": {"action": "function", "function": "reload"}})"));
    Fixture f(core.url(), static_cast<HardwareModel::Enum>(model));

    // the defaults until the core answers
    QCOMPARE(f.controller.buttonIds(), WebviewButtons::ofModel(static_cast<HardwareModel::Enum>(model)));
    QCOMPARE(f.controller.property("buttonIds").toStringList(), f.controller.buttonIds());
    QCOMPARE(f.controller.buttons().value("VOLUME_UP").toMap(), fromJson(R"({"action": "function",
                                                                             "function": "zoom_in"})"));
    for (const QString &button : ownButtons) {
        QVERIFY2(f.controller.buttonIds().contains(button), qPrintable(button));
    }
    for (const QString &button : otherButtons + QStringList{"HOME", "POWER", "VOICE"}) {
        QVERIFY2(!f.controller.buttonIds().contains(button), qPrintable(button));
    }

    QTRY_COMPARE(f.controller.buttons().value("VOLUME_UP").toMap(), fromJson(R"({"action": "pass"})"));
    const QVariantMap buttons = f.controller.buttons();
    QCOMPARE(QStringList(buttons.keys()).size(), f.controller.buttonIds().size());
    for (const QString &button : f.controller.buttonIds()) {
        QVERIFY2(buttons.contains(button), qPrintable(button));
    }
    QCOMPARE(buttons.value("VOLUME_DOWN").toMap(), fromJson(R"({"action": "pass"})"));
    QCOMPARE(buttons.value("BACK").toMap(), fromJson(R"({"action": "function", "function": "close"})"));
    QCOMPARE(buttons.value("DPAD_MIDDLE").toMap(), fromJson(R"({"action": "key", "key": "Enter"})"));
    QCOMPARE(buttons.value("CHANNEL_DOWN").toMap(), fromJson(R"({"action": "function", "function": "page_down"})"));
    if (model == HardwareModel::UCR3) {
        QCOMPARE(buttons.value("MENU").toMap(), fromJson(R"({"action": "function", "function": "reload"})"));
        QVERIFY(!buttons.contains("RED"));
    } else {
        QCOMPARE(buttons.value("RED").toMap(), fromJson(R"({"action": "key", "key": "ColorF0Red"})"));
        QVERIFY(!buttons.contains("MENU"));
    }
    QCOMPARE(f.controller.property("buttons").toMap(), buttons);
    // what the screen and the settings page read
    QCOMPARE(f.controller.buttonFunctions(), WebviewButtons::functions());
    QCOMPARE(f.controller.buttonKeys(), WebviewButtons::keys());
    QCOMPARE(f.controller.defaultButtonAction("VOLUME_UP"), WebviewButtons::defaultAction("VOLUME_UP"));
    QVERIFY(f.controller.buttonRepeats(fromJson(R"({"action": "function", "function": "page_down"})")));
    QVERIFY(!f.controller.buttonRepeats(fromJson(R"({"action": "function", "function": "zoom_in"})")));
}

void testWebviewController::buttons_aChangeSendsOnlyThatButtonAndFollowsTheAnswer() {
    FakeCore core;
    Fixture  f(core.url());
    QTRY_COMPARE(f.globalSettings.size(), 1);  // get_webview_cfg
    QSignalSpy notifications(&f.notification, &uc::ui::Notification::notificationCreated);

    // the action follows the core's answer, not the write
    f.controller.setButtonAction("VOLUME_UP", fromJson(R"({"action": "pass"})"));
    QCOMPARE(f.controller.buttons().value("VOLUME_UP").toMap(),
             fromJson(R"({"action": "function", "function": "zoom_in"})"));
    QTRY_COMPARE(f.controller.buttons().value("VOLUME_UP").toMap(), fromJson(R"({"action": "pass"})"));
    f.controller.setButtonAction("CHANNEL_UP", fromJson(R"({"action": "key", "key": "ChannelUp"})"));
    QTRY_COMPARE(f.controller.buttons().value("CHANNEL_UP").toMap(),
                 fromJson(R"({"action": "key", "key": "ChannelUp"})"));
    // the field the action does not use is not sent
    f.controller.setButtonAction("BACK", fromJson(R"({"action": "function", "function": "back_or_close",
                                                       "key": "Escape"})"));
    QTRY_COMPARE(f.controller.buttons().value("BACK").toMap(),
                 fromJson(R"({"action": "function", "function": "back_or_close"})"));

    // set_webview_cfg with that one button: the core merges it into its buttons
    QCOMPARE(core.settingsUpdates,
             (QList<QVariantMap>{fromJson(R"({"buttons": {"VOLUME_UP": {"action": "pass"}}})"),
                                 fromJson(R"({"buttons": {"CHANNEL_UP": {"action": "key", "key": "ChannelUp"}}})"),
                                 fromJson(R"({"buttons": {"BACK": {"action": "function",
                                                                   "function": "back_or_close"}}})")}));
    // the other buttons keep theirs
    QCOMPARE(f.controller.buttons().value("VOLUME_DOWN").toMap(),
             fromJson(R"({"action": "function", "function": "zoom_out"})"));
    QCOMPARE(notifications.count(), 0);

    // a change made in the web configurator shows at once
    QSignalSpy changed(&f.controller, &WebViewController::webviewCfgChanged);
    core.changeSettings(QVariantMap{{"buttons", fromJson(R"({"VOLUME_UP": {"action": "function",
                                                                          "function": "zoom_reset"}})")}});
    QTRY_COMPARE(changed.count(), 1);
    QCOMPARE(f.controller.buttons().value("VOLUME_UP").toMap(),
             fromJson(R"({"action": "function", "function": "zoom_reset"})"));
    // the core's buttons replace what the UI had: the ones it does not name have their default
    QCOMPARE(f.controller.buttons().value("CHANNEL_UP").toMap(),
             fromJson(R"({"action": "function", "function": "page_up"})"));
}

void testWebviewController::buttons_invalidOrUnchangedSendNothing() {
    FakeCore core;
    Fixture  f(core.url(), HardwareModel::UCR3);
    QTRY_COMPARE(core.declarations.size(), 1);
    QTRY_COMPARE(f.globalSettings.size(), 1);  // get_webview_cfg

    // a fixed button, an unknown one, one of the other model, an invalid action, the action the button has
    f.controller.setButtonAction("HOME", fromJson(R"({"action": "pass"})"));
    f.controller.setButtonAction("SHUFFLE", fromJson(R"({"action": "pass"})"));
    f.controller.setButtonAction("RED", fromJson(R"({"action": "pass"})"));
    f.controller.setButtonAction("MENU", fromJson(R"({"action": "key", "key": "Menu"})"));
    f.controller.setButtonAction("MENU", fromJson(R"({"action": "function"})"));
    f.controller.setButtonAction("BACK", fromJson(R"({"action": "function", "function": "close"})"));
    // a request that would be sent goes before an answer to a later one: wait for a round trip
    f.controller.setButtonAction("MENU", fromJson(R"({"action": "pass"})"));
    QTRY_COMPARE(f.controller.buttons().value("MENU").toMap(), fromJson(R"({"action": "pass"})"));
    QCOMPARE(core.settingsUpdates, QList<QVariantMap>{fromJson(R"({"buttons": {"MENU": {"action": "pass"}}})")});
}

void testWebviewController::clearFromTheSettings_isTheCommandsClear() {
    Fixture f;
    f.clearSupported = true;
    f.clearStart = WebViewController::ClearStart::Started;
    QVERIFY(f.controller.canClearWebsiteData());
    QSignalSpy cleared(&f.controller, &WebViewController::dataCleared);
    QSignalSpy reload(&f.controller, &WebViewController::reloadRequested);

    // the cache, then all website data: reported when the library is done, and every shown page reloads like after
    // the clear_cache and clear_data commands
    f.controller.clearData(false);
    f.controller.clearData(true);
    QCOMPARE(f.clears, (QList<bool>{false, true}));
    QCOMPARE(cleared.count(), 0);
    f.clearDone.takeFirst()(true);
    QCOMPARE(cleared.count(), 1);
    QCOMPARE(cleared.at(0), (QVariantList{false, static_cast<int>(WebViewController::Cleared)}));
    f.clearDone.takeFirst()(true);
    QCOMPARE(cleared.at(1), (QVariantList{true, static_cast<int>(WebViewController::Cleared)}));
    QCOMPARE(reload.count(), 2);
    QVERIFY(reload.at(1).at(0).toString().isEmpty());
    // no command was answered
    QVERIFY(f.codes.isEmpty());
}

void testWebviewController::clearFromTheSettings_reportsAFailure() {
    Fixture f;
    f.clearSupported = true;
    QSignalSpy cleared(&f.controller, &WebViewController::dataCleared);
    QSignalSpy reload(&f.controller, &WebViewController::reloadRequested);

    // the library reports a failure
    f.clearStart = WebViewController::ClearStart::Started;
    f.controller.clearData(true);
    f.clearDone.takeFirst()(false);
    QCOMPARE(cleared.at(0), (QVariantList{true, static_cast<int>(WebViewController::Failed)}));

    // the library refuses the request: reported from inside the call
    f.clearStart = WebViewController::ClearStart::Refused;
    f.controller.clearData(false);
    QCOMPARE(cleared.count(), 2);
    QCOMPARE(cleared.at(1), (QVariantList{false, static_cast<int>(WebViewController::Failed)}));
    QCOMPARE(reload.count(), 0);
}

void testWebviewController::clearFromTheSettings_isUnsupportedWithAnOlderLibrary() {
    Fixture f;
    QVERIFY(!f.controller.canClearWebsiteData());
    QSignalSpy cleared(&f.controller, &WebViewController::dataCleared);
    f.controller.clearData(false);
    QCOMPARE(cleared.count(), 1);
    QCOMPARE(cleared.at(0), (QVariantList{false, static_cast<int>(WebViewController::Unsupported)}));
}

void testWebviewController::optionsEventWithoutSecrets_isNotApplied() {
    Fixture f;
    f.addWebview(fromJson(R"({"url": "https://a.example/", "auth": {"type": "bearer", "token": "secret-1"}})"));
    QCOMPARE(f.webview()->getSite().authHeaderValue(), QStringLiteral("Bearer secret-1"));

    // the event carries the flag, not the token: applying it would drop the token. The entity is fetched instead.
    uc::core::Entity event;
    event.id = kWebviewId;
    event.type = QStringLiteral("Webview");
    event.options = fromJson(R"({"url": "https://b.example/", "auth": {"type": "bearer", "has_token": true}})");
    event.optionsProvided = true;
    f.entities.onEntityChanged(kWebviewId, event);

    QCOMPARE(f.webview()->getUrl(), QUrl("https://a.example/"));
    QCOMPARE(f.webview()->getSite().authHeaderValue(), QStringLiteral("Bearer secret-1"));

    // the core's answer to get_entity carries the token: applied
    uc::core::Entity answer = event;
    answer.optionsProvided = false;
    answer.options = fromJson(R"({"url": "https://b.example/", "auth": {"type": "bearer", "token": "secret-2"}})");
    f.entities.onEntityChanged(kWebviewId, answer);
    QCOMPARE(f.webview()->getUrl(), QUrl("https://b.example/"));
    QCOMPARE(f.webview()->getSite().authHeaderValue(), QStringLiteral("Bearer secret-2"));
}

void testWebviewController::optionsEventWithoutSecretOptions_isApplied() {
    Fixture f;
    f.addWebview(fromJson(R"({"url": "https://a.example/", "zoom": 80})"));

    uc::core::Entity event;
    event.id = kWebviewId;
    event.type = QStringLiteral("Webview");
    event.options = fromJson(R"({"url": "https://a.example/", "zoom": 60})");
    event.optionsProvided = true;
    f.entities.onEntityChanged(kWebviewId, event);
    QCOMPARE(f.webview()->getZoom(), 60);
}

QTEST_MAIN(testWebviewController)

#include "test_webview_controller.moc"
