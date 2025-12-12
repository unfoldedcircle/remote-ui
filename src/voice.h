// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJSEngine>
#include <QJsonDocument>
#include <QObject>
#include <QQmlEngine>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "core/core.h"

namespace uc {

class Voice : public QObject {
    Q_OBJECT

 public:
    explicit Voice(core::Api* core, QObject* parent = nullptr);
    ~Voice();

    Q_INVOKABLE int getSessionId();
    Q_INVOKABLE void playSpeechResponse(const QString& url, const QString &mimeType);

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

    QProcess m_process;

    int m_sessionId = 0;
};
}  // namespace uc
