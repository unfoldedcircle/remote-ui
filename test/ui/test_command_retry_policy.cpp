// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "ui/entity/commandRetryPolicy.h"

using uc::ui::isRepeatingCommand;
using uc::ui::isRequestRejected;
using uc::ui::mayResendAfterWakeup;

// The resume window after a wakeup sends a failed entity command again every 500 ms. These tests cover which
// command and which failure code is allowed into that repetition, see src/ui/entity/commandRetryPolicy.cpp.
class testCommandRetryPolicy : public QObject {
    Q_OBJECT

 private slots:
    void rejected_isNotResent();
    void rejected_isNotResent_data();
    void transient_isResent();
    void transient_isResent_data();
    void voiceStart_followsTheSamePolicy();
    void keyRepeat_isNotResentOnAnyCode();
    void keyRepeat_isOnlyTheRemoteSendWithRepeatCount();
};

void testCommandRetryPolicy::rejected_isNotResent_data() {
    QTest::addColumn<int>("code");

    QTest::newRow("400 bad request") << 400;
    QTest::newRow("401 unauthorized") << 401;
    QTest::newRow("403 forbidden") << 403;
    QTest::newRow("422 unprocessable entity") << 422;
    QTest::newRow("501 not implemented") << 501;
}

void testCommandRetryPolicy::rejected_isNotResent() {
    QFETCH(int, code);

    QVERIFY(isRequestRejected(code));
    QVERIFY(!mayResendAfterWakeup(QStringLiteral("media_player.on"), QVariantMap(), code));
}

void testCommandRetryPolicy::transient_isResent_data() {
    QTest::addColumn<int>("code");

    QTest::newRow("404 entity not registered yet") << 404;
    QTest::newRow("408 request timed out") << 408;
    QTest::newRow("409 conflict") << 409;
    QTest::newRow("500 internal server error") << 500;
    QTest::newRow("503 not connected") << 503;
}

void testCommandRetryPolicy::transient_isResent() {
    QFETCH(int, code);

    QVERIFY(!isRequestRejected(code));
    QVERIFY(mayResendAfterWakeup(QStringLiteral("media_player.on"), QVariantMap(), code));
}

void testCommandRetryPolicy::voiceStart_followsTheSamePolicy() {
    // voice_start has no rule of its own: the session id is created in the UI and travels in the params, so a
    // resend carries the same id instead of asking for another session
    QVariantMap params;
    params.insert(QStringLiteral("session_id"), 8);

    QVERIFY(mayResendAfterWakeup(QStringLiteral("voice_start"), params, 503));
    QVERIFY(mayResendAfterWakeup(QStringLiteral("voice_start"), params, 404));
    QVERIFY(!mayResendAfterWakeup(QStringLiteral("voice_start"), params, 400));

    QVERIFY(mayResendAfterWakeup(QStringLiteral("voice_end"), QVariantMap(), 503));
}

void testCommandRetryPolicy::keyRepeat_isNotResentOnAnyCode() {
    QVariantMap repeat;
    repeat.insert(QStringLiteral("command"), QStringLiteral("VOLUME_UP"));
    repeat.insert(QStringLiteral("repeat"), 3);

    QVERIFY(isRepeatingCommand(QStringLiteral("remote.send"), repeat));
    QVERIFY(!mayResendAfterWakeup(QStringLiteral("remote.send"), repeat, 503));
    QVERIFY(!mayResendAfterWakeup(QStringLiteral("remote.send"), repeat, 404));
}

void testCommandRetryPolicy::keyRepeat_isOnlyTheRemoteSendWithRepeatCount() {
    QVariantMap single;
    single.insert(QStringLiteral("command"), QStringLiteral("VOLUME_UP"));

    // the same command without a repeat count is a single button press and is sent again
    QVERIFY(!isRepeatingCommand(QStringLiteral("remote.send"), single));
    QVERIFY(mayResendAfterWakeup(QStringLiteral("remote.send"), single, 503));
    QVERIFY(!mayResendAfterWakeup(QStringLiteral("remote.send"), single, 400));
}

QTEST_GUILESS_MAIN(testCommandRetryPolicy)

#include "test_command_retry_policy.moc"
