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

    void unknownResponse_settlesTheRequest();
    void unknownResponse_withErrorCode_failsTheRequest();
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

/**
 * A response with a message name this version does not know, e.g. from a newer core, used to cancel the request
 * timeout and then call nothing: the handlers of the request stayed connected for the rest of the process.
 */
void testCoreResponseHandlers::unknownResponse_settlesTheRequest() {
    uc::core::Api api(kTestUrl);

    int successCount = 0;
    int failCount = 0;

    api.onResult(
        7, [&]() { successCount++; },
        [&](int code, QString message) {
            Q_UNUSED(code)
            Q_UNUSED(message)
            failCount++;
        });

    QVariantMap response;
    response.insert(QStringLiteral("kind"), QStringLiteral("resp"));
    response.insert(QStringLiteral("req_id"), 7);
    response.insert(QStringLiteral("msg"), QStringLiteral("message_of_a_future_core"));
    response.insert(QStringLiteral("code"), 200);
    api.processResponseMessage(response);

    QCOMPARE(successCount, 1);
    QCOMPARE(failCount, 0);

    // settled: a second answer is ignored
    api.processResponseMessage(response);
    QCOMPARE(successCount, 1);
}

void testCoreResponseHandlers::unknownResponse_withErrorCode_failsTheRequest() {
    uc::core::Api api(kTestUrl);

    int successCount = 0;
    int failCode = 0;

    api.onResponseWithErrorResult(
        8, &uc::core::Api::respProfile,
        [&](uc::core::Profile profile) {
            Q_UNUSED(profile)
            successCount++;
        },
        [&](int code, QString message) {
            Q_UNUSED(message)
            failCode = code;
        });

    QVariantMap response;
    response.insert(QStringLiteral("kind"), QStringLiteral("resp"));
    response.insert(QStringLiteral("req_id"), 8);
    response.insert(QStringLiteral("msg"), QStringLiteral("message_of_a_future_core"));
    response.insert(QStringLiteral("code"), 500);
    api.processResponseMessage(response);

    QCOMPARE(successCount, 0);
    QCOMPARE(failCode, 500);
}

QTEST_GUILESS_MAIN(testCoreResponseHandlers)

#include "test_core_response_handlers.moc"
