// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "core/core.h"

// an unreachable address: the socket connection attempt is asynchronous and never completes during a test
static const QString kTestUrl = QStringLiteral("ws://127.0.0.1:1/ws");

/**
 * The response handlers of uc::core::Api are one-shots: they must run exactly once and release their signal
 * connection afterwards. A request can be answered more than once, since the request timeout emits a result
 * response and the real (slow) response of the request may still arrive after it.
 */
class testCoreResponseHandlers : public QObject {
    Q_OBJECT

 private slots:
    void onResult_success_runsOnce();
    void onResult_failure_runsOnce();
    void onResult_lateResponseAfterTimeoutIsIgnored();
    void onResult_ignoresRequestThatWasNotSent();

    void onResponseWithErrorResult_success_runsOnce();
    void onResponseWithErrorResult_lateResponseAfterTimeoutIsIgnored();
};

void testCoreResponseHandlers::onResult_success_runsOnce() {
    uc::core::Api api(kTestUrl);

    int successCount = 0;
    int failCount = 0;

    api.onResult(
        1, [&]() { successCount++; },
        [&](int code, QString message) {
            Q_UNUSED(code)
            Q_UNUSED(message)
            failCount++;
        });

    emit api.respResult(1, 200, QString());
    emit api.respResult(1, 200, QString());

    QCOMPARE(successCount, 1);
    QCOMPARE(failCount, 0);
}

void testCoreResponseHandlers::onResult_failure_runsOnce() {
    uc::core::Api api(kTestUrl);

    int successCount = 0;
    int failCount = 0;

    api.onResult(
        2, [&]() { successCount++; },
        [&](int code, QString message) {
            Q_UNUSED(code)
            Q_UNUSED(message)
            failCount++;
        });

    emit api.respResult(2, 503, QStringLiteral("service unavailable"));
    emit api.respResult(2, 503, QStringLiteral("service unavailable"));

    QCOMPARE(successCount, 0);
    QCOMPARE(failCount, 1);
}

void testCoreResponseHandlers::onResult_lateResponseAfterTimeoutIsIgnored() {
    uc::core::Api api(kTestUrl);

    int successCount = 0;
    int failCount = 0;

    api.onResult(
        3, [&]() { successCount++; },
        [&](int code, QString message) {
            Q_UNUSED(code)
            Q_UNUSED(message)
            failCount++;
        });

    // the request timed out
    emit api.respResult(3, 408, QStringLiteral("Request timed out"));
    // the real response of the slow request arrives afterwards
    emit api.respResult(3, 200, QString());

    QCOMPARE(failCount, 1);
    QCOMPARE(successCount, 0);
}

void testCoreResponseHandlers::onResult_ignoresRequestThatWasNotSent() {
    uc::core::Api api(kTestUrl);

    int successCount = 0;
    int failCount = 0;

    api.onResult(
        -1, [&]() { successCount++; },
        [&](int code, QString message) {
            Q_UNUSED(code)
            Q_UNUSED(message)
            failCount++;
        });

    emit api.respResult(-1, 200, QString());

    QCOMPARE(successCount, 0);
    QCOMPARE(failCount, 0);
}

void testCoreResponseHandlers::onResponseWithErrorResult_success_runsOnce() {
    uc::core::Api api(kTestUrl);

    int successCount = 0;
    int failCount = 0;

    uc::core::Profile profile;
    profile.id = QStringLiteral("profile-1");

    api.onResponseWithErrorResult(
        4, &uc::core::Api::respProfile,
        [&](uc::core::Profile responseProfile) {
            Q_UNUSED(responseProfile)
            successCount++;
        },
        [&](int code, QString message) {
            Q_UNUSED(code)
            Q_UNUSED(message)
            failCount++;
        });

    emit api.respProfile(4, 200, profile);
    emit api.respProfile(4, 200, profile);

    QCOMPARE(successCount, 1);
    QCOMPARE(failCount, 0);
}

void testCoreResponseHandlers::onResponseWithErrorResult_lateResponseAfterTimeoutIsIgnored() {
    uc::core::Api api(kTestUrl);

    int successCount = 0;
    int failCount = 0;

    uc::core::Profile profile;

    api.onResponseWithErrorResult(
        5, &uc::core::Api::respProfile,
        [&](uc::core::Profile responseProfile) {
            Q_UNUSED(responseProfile)
            successCount++;
        },
        [&](int code, QString message) {
            Q_UNUSED(code)
            Q_UNUSED(message)
            failCount++;
        });

    // the request timed out
    emit api.respResult(5, 408, QStringLiteral("Request timed out"));
    // the real response of the slow request arrives afterwards
    emit api.respProfile(5, 200, profile);

    QCOMPARE(failCount, 1);
    QCOMPARE(successCount, 0);
}

QTEST_GUILESS_MAIN(testCoreResponseHandlers)

#include "test_core_response_handlers.moc"
