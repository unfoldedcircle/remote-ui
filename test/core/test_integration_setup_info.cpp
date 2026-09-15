// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QJsonDocument>
#include <QtTest>

#include "core/core.h"

using uc::core::Api;
using uc::core::IntegrationEnums;
using uc::core::IntegrationSetupInfo;

/**
 * Parsing of the integration setup info: the setup responses and the integration_setup_change event share the
 * object, the event names the session by driver_id. Covers the keep-alive lease, the driver provided error message
 * and the battery budget fields.
 */
class testIntegrationSetupInfo : public QObject {
    Q_OBJECT

 private slots:
    void parse_response_withKeepaliveAndLanguage();
    void parse_event_usesDriverIdAndErrorMessage();
    void parse_rejectedInput_keepsPageAndRawUserAction();
    void parse_batteryLimit();
    void parse_batteryLimit_defaultsReasonToBattery();
    void parse_missingError_isNone();
    void parse_confirmationPage();

 private:
    static QVariantMap fromJson(const char *json) {
        return QJsonDocument::fromJson(json).toVariant().toMap();
    }
};

void testIntegrationSetupInfo::parse_response_withKeepaliveAndLanguage() {
    IntegrationSetupInfo info = Api::parseIntegrationSetupInfo(fromJson(R"({
        "id": "uc:hue", "state": "NEW", "keepalive_timeout_sec": 300, "language": "de_CH",
        "setup_limit_active": false
    })"));

    QCOMPARE(info.id, QStringLiteral("uc:hue"));
    QCOMPARE(info.state, IntegrationEnums::SetupState::NEW);
    QCOMPARE(info.error, IntegrationEnums::SetupError::NONE);
    QCOMPARE(info.keepaliveTimeoutSec, 300);
    QCOMPARE(info.requireUserAction, false);
    QCOMPARE(info.setupLimitActive, false);
    QCOMPARE(info.setupLimitReason, IntegrationEnums::SetupLimitReason::NO_LIMIT);
}

void testIntegrationSetupInfo::parse_event_usesDriverIdAndErrorMessage() {
    IntegrationSetupInfo info = Api::parseIntegrationSetupInfo(fromJson(R"({
        "event_type": "STOP", "driver_id": "mock", "state": "ERROR", "error": "DRIVER_UNAVAILABLE",
        "error_message": {"en": "Connection attempts exhausted", "de": "Verbindungsversuche erschöpft"},
        "setup_limit_active": false
    })"));

    QCOMPARE(info.id, QStringLiteral("mock"));
    QCOMPARE(info.state, IntegrationEnums::SetupState::ERROR);
    QCOMPARE(info.error, IntegrationEnums::SetupError::DRIVER_UNAVAILABLE);
    QCOMPARE(info.errorMessage.value("en").toString(), QStringLiteral("Connection attempts exhausted"));
    QCOMPARE(info.errorMessage.value("de").toString(), QStringLiteral("Verbindungsversuche erschöpft"));
    // an event without lease information must not restart the keep-alive
    QCOMPARE(info.keepaliveTimeoutSec, 0);
}

void testIntegrationSetupInfo::parse_rejectedInput_keepsPageAndRawUserAction() {
    const char *page = R"({
        "event_type": "SETUP", "driver_id": "mock", "state": "WAIT_USER_ACTION", "error": "INVALID_INPUT",
        "error_message": {"en": "Wrong PIN"},
        "require_user_action": {"input": {"title": {"en": "PIN"},
            "settings": [{"id": "pin", "label": {"en": "PIN"}, "field": {"text": {"value": ""}}}]}},
        "setup_limit_active": false
    })";
    IntegrationSetupInfo info = Api::parseIntegrationSetupInfo(fromJson(page));

    QCOMPARE(info.state, IntegrationEnums::SetupState::WAIT_USER_ACTION);
    QCOMPARE(info.error, IntegrationEnums::SetupError::INVALID_INPUT);
    QVERIFY(info.requireUserAction);
    QCOMPARE(info.settingsPage.title.value("en").toString(), QStringLiteral("PIN"));
    QCOMPARE(info.settingsPage.settings.size(), 1);
    QVERIFY(info.confirmationPage.title.isEmpty());

    // the raw object identifies a repeated page: the same page again compares equal
    IntegrationSetupInfo again = Api::parseIntegrationSetupInfo(fromJson(page));
    QVERIFY(!info.userAction.isEmpty());
    QCOMPARE(info.userAction, again.userAction);
}

void testIntegrationSetupInfo::parse_batteryLimit() {
    IntegrationSetupInfo info = Api::parseIntegrationSetupInfo(fromJson(R"({
        "id": "mock", "state": "SETUP", "keepalive_timeout_sec": 300,
        "setup_limit_active": true, "setup_expires_in_sec": 1234, "setup_limit_total_sec": 1800,
        "setup_limit_reason": "LOW_BATTERY"
    })"));

    QVERIFY(info.setupLimitActive);
    QCOMPARE(info.setupExpiresInSec, 1234);
    QCOMPARE(info.setupLimitTotalSec, 1800);
    QCOMPARE(info.setupLimitReason, IntegrationEnums::SetupLimitReason::LOW_BATTERY);
}

void testIntegrationSetupInfo::parse_batteryLimit_defaultsReasonToBattery() {
    IntegrationSetupInfo info = Api::parseIntegrationSetupInfo(fromJson(R"({
        "id": "mock", "state": "SETUP", "setup_limit_active": true, "setup_expires_in_sec": 60
    })"));

    QVERIFY(info.setupLimitActive);
    QCOMPARE(info.setupExpiresInSec, 60);
    QCOMPARE(info.setupLimitReason, IntegrationEnums::SetupLimitReason::BATTERY);
}

void testIntegrationSetupInfo::parse_missingError_isNone() {
    // an older core omits the error field, and also the lease and limit fields
    IntegrationSetupInfo info = Api::parseIntegrationSetupInfo(fromJson(R"({"id": "mock", "state": "OK"})"));

    QCOMPARE(info.state, IntegrationEnums::SetupState::OK);
    QCOMPARE(info.error, IntegrationEnums::SetupError::NONE);
    QVERIFY(info.errorMessage.isEmpty());
    QCOMPARE(info.keepaliveTimeoutSec, 0);
    QCOMPARE(info.setupLimitActive, false);
}

void testIntegrationSetupInfo::parse_confirmationPage() {
    IntegrationSetupInfo info = Api::parseIntegrationSetupInfo(fromJson(R"({
        "event_type": "SETUP", "driver_id": "mock", "state": "WAIT_USER_ACTION",
        "require_user_action": {"confirmation": {"title": {"en": "Confirm"}, "message1": {"en": "Press the button"},
            "image": "data:image/png;base64,AAAA", "message2": {"en": "then continue"}}}
    })"));

    QVERIFY(info.requireUserAction);
    QVERIFY(info.settingsPage.settings.isEmpty());
    QCOMPARE(info.confirmationPage.title.value("en").toString(), QStringLiteral("Confirm"));
    QCOMPARE(info.confirmationPage.message1.value("en").toString(), QStringLiteral("Press the button"));
    QCOMPARE(info.confirmationPage.image, QStringLiteral("data:image/png;base64,AAAA"));
    QCOMPARE(info.confirmationPage.message2.value("en").toString(), QStringLiteral("then continue"));
}

QTEST_GUILESS_MAIN(testIntegrationSetupInfo)

#include "test_integration_setup_info.moc"
