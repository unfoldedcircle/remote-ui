// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QJsonDocument>
#include <QtTest>

#include "core/core.h"

using uc::core::Api;
using uc::core::cfgWebview;
using uc::core::UiEntityCommand;

// for QSignalSpy and QVariant::value()
Q_DECLARE_METATYPE(uc::core::UiEntityCommand)
Q_DECLARE_METATYPE(uc::core::cfgWebview)

// an unreachable address: the socket connection attempt is asynchronous and never completes during a test
static const QString kTestUrl = QStringLiteral("ws://127.0.0.1:1/ws");

/**
 * The messages of the UI app with the core for the entity types the UI executes (web view entities): set_ui_info,
 * the core's entity_command request and its result response, and the global web view settings.
 */
class testUiEntityCommands : public QObject {
    Q_OBJECT

 private slots:
    void uiInfo_carriesTheVersionAndTheEntityTypes();
    void entityCommand_isParsed();
    void entityCommand_requestIsSignalledWithItsId();
    void unknownRequest_isIgnored();
    void result_success_hasNoErrorObject();
    void result_error_carriesCodeWordAndMessage();
    void webviewCfg_isParsed();
    void webviewCfg_unknownValuesKeepTheDefaults();
    void webviewCfg_responseIsSignalled();

 private:
    static QVariantMap fromJson(const char *json) { return QJsonDocument::fromJson(json).toVariant().toMap(); }
};

void testUiEntityCommands::uiInfo_carriesTheVersionAndTheEntityTypes() {
    const QVariantMap msgData = Api::uiInfoMsgData(QStringLiteral("1.2.3"), QStringList{QStringLiteral("webview")});
    QCOMPARE(msgData.value("version").toString(), QStringLiteral("1.2.3"));
    QCOMPARE(msgData.value("entity_types").toStringList(), QStringList{QStringLiteral("webview")});

    // an empty list withdraws the declaration: the key is still sent, the core requires it
    const QVariantMap none = Api::uiInfoMsgData(QStringLiteral("1.2.3"), QStringList());
    QVERIFY(none.contains("entity_types"));
    QVERIFY(none.value("entity_types").toStringList().isEmpty());
}

void testUiEntityCommands::entityCommand_isParsed() {
    const UiEntityCommand command = Api::parseUiEntityCommand(42, fromJson(R"({
        "entity_id": "uc_web.main.0f3b", "entity_type": "webview", "cmd_id": "open", "params": {"a": 1}
    })"));

    QCOMPARE(command.reqId, 42);
    QCOMPARE(command.entityId, QStringLiteral("uc_web.main.0f3b"));
    QCOMPARE(command.entityType, QStringLiteral("webview"));
    QCOMPARE(command.cmdId, QStringLiteral("open"));
    QCOMPARE(command.params.value("a").toInt(), 1);

    const UiEntityCommand bare = Api::parseUiEntityCommand(7, fromJson(R"({"entity_id": "x"})"));
    QVERIFY(bare.cmdId.isEmpty());
    QVERIFY(bare.params.isEmpty());
}

void testUiEntityCommands::entityCommand_requestIsSignalledWithItsId() {
    qRegisterMetaType<UiEntityCommand>();
    Api        api(kTestUrl);
    QSignalSpy spy(&api, &Api::uiEntityCommand);

    // the core's request carries the id in "id", the answer goes back as "req_id"
    api.processRequestMessage(fromJson(R"({
        "kind": "req", "id": 17, "msg": "entity_command",
        "msg_data": {"entity_id": "uc_web.main.1", "entity_type": "webview", "cmd_id": "reload"}
    })"));

    QCOMPARE(spy.count(), 1);
    const auto command = spy.first().first().value<UiEntityCommand>();
    QCOMPARE(command.reqId, 17);
    QCOMPARE(command.entityId, QStringLiteral("uc_web.main.1"));
    QCOMPARE(command.cmdId, QStringLiteral("reload"));
}

void testUiEntityCommands::unknownRequest_isIgnored() {
    Api        api(kTestUrl);
    QSignalSpy spy(&api, &Api::uiEntityCommand);
    api.processRequestMessage(fromJson(R"({"kind": "req", "id": 3, "msg": "something_new", "msg_data": {}})"));
    QCOMPARE(spy.count(), 0);
}

void testUiEntityCommands::result_success_hasNoErrorObject() {
    const QVariantMap response = Api::resultResponse(42, 200, QString());
    QCOMPARE(response.value("kind").toString(), QStringLiteral("resp"));
    QCOMPARE(response.value("req_id").toInt(), 42);
    QCOMPARE(response.value("msg").toString(), QStringLiteral("result"));
    QCOMPARE(response.value("code").toInt(), 200);
    QVERIFY(response.value("msg_data").toMap().isEmpty());
}

void testUiEntityCommands::result_error_carriesCodeWordAndMessage() {
    const QVariantMap notFound = Api::resultResponse(5, 404, QStringLiteral("Unknown web view"));
    QCOMPARE(notFound.value("code").toInt(), 404);
    QCOMPARE(notFound.value("msg_data").toMap().value("code").toString(), QStringLiteral("NOT_FOUND"));
    QCOMPARE(notFound.value("msg_data").toMap().value("message").toString(), QStringLiteral("Unknown web view"));

    QCOMPARE(Api::resultResponse(5, 400, "x").value("msg_data").toMap().value("code").toString(),
             QStringLiteral("BAD_REQUEST"));
    QCOMPARE(Api::resultResponse(5, 500, "x").value("msg_data").toMap().value("code").toString(),
             QStringLiteral("INTERNAL_SERVER_ERROR"));
    QCOMPARE(Api::resultResponse(5, 501, "x").value("msg_data").toMap().value("code").toString(),
             QStringLiteral("NOT_IMPLEMENTED"));
    QCOMPARE(Api::resultResponse(5, 503, "x").value("msg_data").toMap().value("code").toString(),
             QStringLiteral("SERVICE_UNAVAILABLE"));
}

void testUiEntityCommands::webviewCfg_isParsed() {
    const cfgWebview cfg = Api::parseWebviewCfg(fromJson(R"({
        "allow_invalid_certificates": true, "color_scheme": "light", "website_data": "clear_all_on_close",
        "buttons": {"VOLUME_UP": {"action": "pass"}, "CHANNEL_UP": {"action": "key", "key": "ChannelUp"}}
    })"));
    QVERIFY(cfg.allowInvalidCertificates);
    QCOMPARE(cfg.colorScheme, QStringLiteral("light"));
    QCOMPARE(cfg.websiteData, QStringLiteral("clear_all_on_close"));
    // the buttons as the core sent them: WebViewController validates them and adds the defaults
    QCOMPARE(cfg.buttons, fromJson(R"({"VOLUME_UP": {"action": "pass"},
                                       "CHANNEL_UP": {"action": "key", "key": "ChannelUp"}})"));
}

void testUiEntityCommands::webviewCfg_unknownValuesKeepTheDefaults() {
    const cfgWebview cfg =
        Api::parseWebviewCfg(fromJson(R"({"color_scheme": "sepia", "website_data": "forget", "buttons": "none"})"));
    QVERIFY(!cfg.allowInvalidCertificates);
    QCOMPARE(cfg.colorScheme, QStringLiteral("dark"));
    QCOMPARE(cfg.websiteData, QStringLiteral("keep"));
    QVERIFY(cfg.buttons.isEmpty());

    // a core without the fields: the defaults
    const cfgWebview empty = Api::parseWebviewCfg(QVariantMap());
    QCOMPARE(empty.colorScheme, QStringLiteral("dark"));
    QCOMPARE(empty.websiteData, QStringLiteral("keep"));
    QVERIFY(empty.buttons.isEmpty());
}

void testUiEntityCommands::webviewCfg_responseIsSignalled() {
    // the signal names the type as core::cfgWebview
    qRegisterMetaType<cfgWebview>("core::cfgWebview");
    Api        api(kTestUrl);
    QSignalSpy spy(&api, &Api::respWebviewCfg);

    api.processResponseMessage(fromJson(R"({
        "kind": "resp", "req_id": 9, "code": 200, "msg": "webview_cfg",
        "msg_data": {"allow_invalid_certificates": false, "color_scheme": "light", "website_data": "keep"}
    })"));

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(0).toInt(), 9);
    QCOMPARE(spy.first().at(2).value<cfgWebview>().colorScheme, QStringLiteral("light"));
}

QTEST_MAIN(testUiEntityCommands)

#include "test_ui_entity_commands.moc"
