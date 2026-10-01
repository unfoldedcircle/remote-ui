// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJSEngine>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QPointer>
#include <QProcess>
#include <QQmlEngine>

#include "core/core.h"

namespace uc {

class Voice : public QObject {
    Q_OBJECT

 public:
    explicit Voice(core::Api* core, QObject* parent = nullptr);
    ~Voice();

    Q_INVOKABLE int getSessionId();
    /**
     * Plays the spoken answer of the assistant: downloads it from `url` and streams it into the player.
     * assistantAudioSpeechResponseEnd() is emitted once when this playback ends, or could not start. A playback
     * that is still running is stopped first, without reporting its end: the end of the new one follows.
     */
    Q_INVOKABLE void playSpeechResponse(const QString& url, const QString &mimeType);

    /**
     * Stops the answer that is playing, without reporting its end. A new voice question stops it: the voice
     * overlay can be closed while the answer goes on, and the end of that answer used to close the overlay of
     * the next question.
     */
    Q_INVOKABLE void stopSpeechResponse();

    /// The player the answer is streamed into, on its standard input. ffplay by default; for the unit tests.
    void setPlayer(const QString& program, const QStringList& arguments);

    static QObject* qmlInstance(QQmlEngine* engine, QJSEngine* scriptEngine);

 signals:
    void assistantEventReady(QString entityId, int sessionId);
    void assistantEventSttResponse(QString entityId, int sessionId, QString text);
    void assistantEventTextResponse(QString entityId, int sessionId, bool success, QString text);
    void assistantEventSpeechResponse(QString entityId, int sessionId, QString url, QString mimeType);
    void assistantEventFinished(QString entityId, int sessionId);
    void assistantEventError(QString entityId, int sessionId, QString message);
    void assistantAudioSpeechResponseEnd();

 public slots:
    void onAssistantEventReady(const QString& entityId, int sesssionId);
    void onAssistantEventSttResponse(QString entityId, int sessionId, QString text);
    void onAssistantEventTextResponse(QString entityId, int sessionId, bool success, QString text);
    void onAssistantEventSpeechResponse(QString entityId, int sessionId, QString url, QString mimeType);
    void onAssistantEventFinished(QString entityId, int sessionId);
    void onAssistantEventError(QString entityId, int sessionId, core::AssistantErrorCodes::Enum code, QString message);

 private slots:

 private:
    static Voice* s_instance;

    core::Api* m_core;

    QString     m_playerProgram;
    QStringList m_playerArguments;
    // A player and a download per playback: the previous player is killed and deleted once it has exited, no
    // waiting on the UI thread, and nothing of the previous playback can reach the current one.
    QPointer<QProcess>      m_player;
    QPointer<QNetworkReply> m_speechReply;
    QNetworkAccessManager   m_networkManager;

    int m_sessionId = 0;
};
}  // namespace uc
