// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "voice.h"

#include "logging.h"

namespace uc {

static constexpr int IMAGE_REQUEST_TIMEOUT_MS = 15000;

Voice *Voice::s_instance = nullptr;

Voice::Voice(core::Api *core, QObject *parent) : QObject(parent), m_core(core) {
    Q_ASSERT(s_instance == nullptr);
    s_instance = this;

    QObject::connect(m_core, &core::Api::assistantEventReady, this, &Voice::onAssistantEventReady);
    QObject::connect(m_core, &core::Api::assistantEventSttResponse, this, &Voice::onAssistantEventSttResponse);
    QObject::connect(m_core, &core::Api::assistantEventTextResponse, this, &Voice::onAssistantEventTextResponse);
    QObject::connect(m_core, &core::Api::assistantEventSpeechResponse, this, &Voice::onAssistantEventSpeechResponse);
    QObject::connect(m_core, &core::Api::assistantEventFinished, this, &Voice::onAssistantEventFinished);
    QObject::connect(m_core, &core::Api::assistantEventError, this, &Voice::onAssistantEventError);

    QObject::connect(&m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     this, [=]{
                         emit assistantAudioSpeechResponseEnd();
                     });

    qmlRegisterSingletonType<Voice>("Voice", 1, 0, "Voice", &Voice::qmlInstance);
}

Voice::~Voice() {
    s_instance = nullptr;
}

int Voice::getSessionId()
{
    m_sessionId++;
    return m_sessionId;
}

void Voice::playSpeechResponse(const QString &url, const QString &mimeType)
{
    static const QSet<QString> allowedMimes = {
        QStringLiteral("audio/mpeg"),
        QStringLiteral("audio/mp3"),
        QStringLiteral("audio/wav"),
        QStringLiteral("audio/x-wav"),
        QStringLiteral("audio/ogg"),
        QStringLiteral("audio/opus"),
        QStringLiteral("audio/webm"),
        QStringLiteral("audio/flac"),
        QStringLiteral("audio/aac")
    };

    if (!allowedMimes.contains(mimeType)) {
        qCWarning(lcVoice()) << "Refusing to play unsupported MIME type:" << mimeType;
        emit assistantAudioSpeechResponseEnd();
        return;
    }

    if (m_process.state() != QProcess::NotRunning) {
        m_process.kill();
        m_process.waitForFinished();
    }

    QStringList args;
    args
        << "-nodisp"
        << "-autoexit"
        << "-";

    m_process.start(QStringLiteral("/usr/bin/ffplay"), args);

    if (!m_process.waitForStarted(5000)) {
        qCWarning(lcVoice()) << "Failed to start ffplay";
        emit assistantAudioSpeechResponseEnd();
        return;
    }

    auto nam   = new QNetworkAccessManager(this);

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);
    request.setTransferTimeout(IMAGE_REQUEST_TIMEOUT_MS);

    // Create SSL configuration that ignores certificate errors
    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);

    request.setSslConfiguration(sslConfig);

    auto reply = nam->get(request);

    connect(reply, &QNetworkReply::readyRead, this, [this, reply]() {
        QByteArray chunk = reply->readAll();
        if (!chunk.isEmpty())
            m_process.write(chunk);
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        m_process.closeWriteChannel();
    });
}

QObject *Voice::qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(scriptEngine)

    QObject *obj = s_instance;
    engine->setObjectOwnership(obj, QQmlEngine::CppOwnership);

    return obj;
}

void Voice::onAssistantEventReady(const QString &entityId, int sesssionId)
{
    emit assistantEventReady(entityId, sesssionId);
}

void Voice::onAssistantEventSttResponse(QString entityId, int sessionId, QString text)
{
    emit assistantEventSttResponse(entityId, sessionId, text);
}

void Voice::onAssistantEventTextResponse(QString entityId, int sessionId, bool success, QString text)
{
    emit assistantEventTextResponse(entityId, sessionId, success, text);
}

void Voice::onAssistantEventSpeechResponse(QString entityId, int sessionId, QString url, QString mimeType)
{
    emit assistantEventSpeechResponse(entityId, sessionId, url, mimeType);
}

void Voice::onAssistantEventFinished(QString entityId, int sessionId)
{
    emit assistantEventFinished(entityId, sessionId);
}

void Voice::onAssistantEventError(QString entityId, int sessionId, core::AssistantErrorCodes::Enum code, QString message)
{
    QString errorMsg;

    qWarning() << code << message;

    switch (code) {
        case core::AssistantErrorCodes::Enum::SERVICE_UNAVAILABLE:
            errorMsg = tr("The service is temporarily unavailable.");
            break;
        case core::AssistantErrorCodes::Enum::INVALID_AUDIO:
            errorMsg = tr("Incorrect audio format.");
            break;
        case core::AssistantErrorCodes::Enum::NO_TEXT_RECOGNIZED:
            errorMsg = tr("I didn’t catch any text from your input. Could you repeat that?");
            break;
        case core::AssistantErrorCodes::Enum::INTENT_FAILED:
            errorMsg = tr("Please try rephrasing your request.");
            break;
        case core::AssistantErrorCodes::Enum::TTS_FAILED:
            errorMsg = tr("I couldn’t generate the audio response.");
            break;
        case core::AssistantErrorCodes::Enum::TIMEOUT:
            errorMsg = tr("It’s taking longer than expected. Please try your request again.");
            break;
        case core::AssistantErrorCodes::Enum::UNEXPECTED_ERROR:
            errorMsg = tr("Something went wrong on our side. Please try again.");
            break;
    }

    emit assistantEventError(entityId, sessionId, errorMsg);
}
}  // namespace uc
