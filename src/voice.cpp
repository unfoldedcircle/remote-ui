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

    setPlayer(QStringLiteral("/usr/bin/ffplay"), {"-nodisp", "-autoexit", "-"});

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

void Voice::setPlayer(const QString &program, const QStringList &arguments) {
    m_playerProgram = program;
    m_playerArguments = arguments;
}

void Voice::playSpeechResponse(const QString &url, const QString &mimeType) {
    static const QSet<QString> allowedMimes = {
        QStringLiteral("audio/mpeg"),  QStringLiteral("audio/mp3"),  QStringLiteral("audio/wav"),
        QStringLiteral("audio/x-wav"), QStringLiteral("audio/ogg"),  QStringLiteral("audio/opus"),
        QStringLiteral("audio/webm"),  QStringLiteral("audio/flac"), QStringLiteral("audio/aac")};

    // A new answer replaces the one that is still playing. Its download fed the new player and its end, reported
    // when it was killed, closed the voice overlay of the new answer.
    stopSpeechResponse();

    if (!allowedMimes.contains(mimeType)) {
        qCWarning(lcVoice()) << "Refusing to play unsupported MIME type:" << mimeType;
        emit assistantAudioSpeechResponseEnd();
        return;
    }

    qCDebug(lcVoice()) << "Speech response: playing" << mimeType;

    auto player = new QProcess(this);
    m_player = player;

    QObject::connect(player, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
                     [this, player](int exitCode, QProcess::ExitStatus exitStatus) {
                         player->deleteLater();
                         if (player != m_player) {
                             return;
                         }
                         qCDebug(lcVoice()) << "Speech response: playback finished" << exitCode << exitStatus;
                         m_player = nullptr;
                         emit assistantAudioSpeechResponseEnd();
                     });

    QObject::connect(player, &QProcess::errorOccurred, this, [this, player](QProcess::ProcessError error) {
        // the other errors end with finished(), which reports the end
        if (error != QProcess::FailedToStart || player != m_player) {
            return;
        }
        qCWarning(lcVoice()) << "Failed to start the speech player" << m_playerProgram << player->errorString();
        player->deleteLater();
        stopSpeechResponse();
        emit assistantAudioSpeechResponseEnd();
    });

    // the answer is streamed into the player once it runs
    QObject::connect(player, &QProcess::started, this, [this, player, url]() {
        if (player != m_player) {
            // replaced while it was starting: it would wait for input forever
            player->kill();
            return;
        }

        QNetworkRequest request(url);
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);
        request.setTransferTimeout(IMAGE_REQUEST_TIMEOUT_MS);

        // Create SSL configuration that ignores certificate errors
        QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        request.setSslConfiguration(sslConfig);

        QNetworkReply *reply = m_networkManager.get(request);
        m_speechReply = reply;

        QObject::connect(reply, &QNetworkReply::readyRead, this, [this, reply, player]() {
            const QByteArray chunk = reply->readAll();
            if (reply != m_speechReply || player != m_player || chunk.isEmpty()) {
                return;
            }
            player->write(chunk);
        });

        QObject::connect(reply, &QNetworkReply::finished, this, [this, reply, player]() {
            reply->deleteLater();
            if (reply != m_speechReply) {
                return;
            }
            m_speechReply = nullptr;
            if (reply->error() != QNetworkReply::NoError) {
                qCWarning(lcVoice()) << "Speech response download failed:" << reply->errorString();
            }
            // the player ends once it has played what it got
            if (player == m_player) {
                player->closeWriteChannel();
            }
        });
    });

    player->start(m_playerProgram, m_playerArguments);
}

void Voice::stopSpeechResponse() {
    if (m_speechReply) {
        QNetworkReply *reply = m_speechReply;
        m_speechReply = nullptr;
        reply->disconnect(this);
        reply->abort();
        reply->deleteLater();
    }

    if (m_player) {
        QProcess *player = m_player;
        m_player = nullptr;
        qCDebug(lcVoice()) << "Speech response: stopping the previous playback";
        // its finished() no longer reports an end, see the handlers in playSpeechResponse(); deleted once it exited
        if (player->state() == QProcess::NotRunning) {
            player->deleteLater();
        } else {
            player->kill();
        }
    }
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
