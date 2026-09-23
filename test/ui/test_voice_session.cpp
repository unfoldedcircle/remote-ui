// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QHash>
#include <QtTest>

#include "ui/entity/voiceSession.h"

using uc::ui::endsVoiceSession;
using uc::ui::isVoiceStartOfEndedSession;
using uc::ui::kAnyVoiceSession;
using uc::ui::startsVoiceSession;
using uc::ui::voiceSessionIdOf;

namespace {

/// The fields of EntityController::pendingCommand that decide whether a voice session end makes it obsolete
struct PendingCommand {
    QString     entityId;
    QString     command;
    QVariantMap params;
};

/// The parameters VoiceAssistant::voiceStart() builds for a session
QVariantMap voiceStartParams(int sessionId) {
    QVariantMap params;
    params.insert(QStringLiteral("session_id"), sessionId);
    params.insert(QStringLiteral("speech_response"), true);
    params.insert(QStringLiteral("timeout"), 15);
    return params;
}

/// The sweep EntityController::cancelPendingVoiceStart() runs over its pending commands
QHash<QString, PendingCommand> cancelPendingVoiceStart(QHash<QString, PendingCommand> pending,
                                                       const QString& entityId, int sessionId) {
    for (auto it = pending.begin(); it != pending.end();) {
        if (isVoiceStartOfEndedSession(it.value().entityId, it.value().command, it.value().params, entityId,
                                       sessionId)) {
            it = pending.erase(it);
        } else {
            ++it;
        }
    }
    return pending;
}

const QString kAssistant = QStringLiteral("uc.main:voice");
const QString kStartId   = QStringLiteral("uc.main:voice.voice_start#1");
const QString kVolumeId  = QStringLiteral("uc.main:voice.media_player.volume_up#1");

}  // namespace

/**
 * A voice_start that failed around a wakeup keeps being sent again for the resume window. When the session it
 * opens has ended in the meantime - the user let go of the microphone button, or the overlay closed after an
 * error or a timeout - the pending start is dropped, so the repetition cannot make the assistant start
 * listening afterwards. See src/ui/entity/voiceSession.cpp.
 */
class testVoiceSession : public QObject {
    Q_OBJECT

 private slots:
    void endedSession_dropsItsPendingStart();
    void endedSession_keepsAnotherPendingCommandOfTheSameEntity();
    void endedSession_keepsTheStartOfAnotherSession();
    void endedSession_keepsTheStartOfAnotherEntity();
    void sessionEndWithoutId_dropsEveryPendingStartOfTheEntity();
    void voiceSessionIdOf_readsTheStartParameters();
    void commandNames_areTheOnesTheVoiceEntitySends();
};

void testVoiceSession::endedSession_dropsItsPendingStart() {
    QHash<QString, PendingCommand> pending;
    pending.insert(kStartId, {kAssistant, QStringLiteral("voice_start"), voiceStartParams(7)});

    const auto remaining = cancelPendingVoiceStart(pending, kAssistant, 7);

    QVERIFY(!remaining.contains(kStartId));
    QVERIFY(remaining.isEmpty());
}

void testVoiceSession::endedSession_keepsAnotherPendingCommandOfTheSameEntity() {
    QHash<QString, PendingCommand> pending;
    pending.insert(kStartId, {kAssistant, QStringLiteral("voice_start"), voiceStartParams(7)});
    pending.insert(kVolumeId, {kAssistant, QStringLiteral("media_player.volume_up"), QVariantMap()});

    const auto remaining = cancelPendingVoiceStart(pending, kAssistant, 7);

    QVERIFY(!remaining.contains(kStartId));
    QVERIFY(remaining.contains(kVolumeId));
    QCOMPARE(remaining.count(), 1);
}

void testVoiceSession::endedSession_keepsTheStartOfAnotherSession() {
    QHash<QString, PendingCommand> pending;
    pending.insert(kStartId, {kAssistant, QStringLiteral("voice_start"), voiceStartParams(8)});

    const auto remaining = cancelPendingVoiceStart(pending, kAssistant, 7);

    QVERIFY(remaining.contains(kStartId));
}

void testVoiceSession::endedSession_keepsTheStartOfAnotherEntity() {
    QHash<QString, PendingCommand> pending;
    pending.insert(kStartId, {QStringLiteral("uc.other:voice"), QStringLiteral("voice_start"), voiceStartParams(7)});

    const auto remaining = cancelPendingVoiceStart(pending, kAssistant, 7);

    QVERIFY(remaining.contains(kStartId));
}

void testVoiceSession::sessionEndWithoutId_dropsEveryPendingStartOfTheEntity() {
    // voice_end carries no parameters today, so the session end that travels through the entity controller
    // names no session id and every start of that assistant which is still pending belongs to it
    QHash<QString, PendingCommand> pending;
    pending.insert(kStartId, {kAssistant, QStringLiteral("voice_start"), voiceStartParams(7)});
    pending.insert(kVolumeId, {kAssistant, QStringLiteral("media_player.volume_up"), QVariantMap()});

    QCOMPARE(voiceSessionIdOf(QVariantMap()), kAnyVoiceSession);

    const auto remaining = cancelPendingVoiceStart(pending, kAssistant, voiceSessionIdOf(QVariantMap()));

    QVERIFY(!remaining.contains(kStartId));
    QVERIFY(remaining.contains(kVolumeId));
}

void testVoiceSession::voiceSessionIdOf_readsTheStartParameters() {
    QCOMPARE(voiceSessionIdOf(voiceStartParams(7)), 7);
    QCOMPARE(voiceSessionIdOf(QVariantMap()), kAnyVoiceSession);

    QVariantMap unusable;
    unusable.insert(QStringLiteral("session_id"), QStringLiteral("not a number"));
    QCOMPARE(voiceSessionIdOf(unusable), kAnyVoiceSession);
}

void testVoiceSession::commandNames_areTheOnesTheVoiceEntitySends() {
    // entity::Base::sendCommand() lower-cases the command and sends a voice assistant command without the
    // entity type prefix every other entity gets
    QVERIFY(startsVoiceSession(QStringLiteral("voice_start")));
    QVERIFY(!startsVoiceSession(QStringLiteral("voice_end")));
    QVERIFY(endsVoiceSession(QStringLiteral("voice_end")));
    QVERIFY(!endsVoiceSession(QStringLiteral("voice_start")));
    QVERIFY(!startsVoiceSession(QStringLiteral("media_player.volume_up")));
    QVERIFY(!endsVoiceSession(QStringLiteral("media_player.volume_up")));
}

QTEST_GUILESS_MAIN(testVoiceSession)

#include "test_voice_session.moc"
