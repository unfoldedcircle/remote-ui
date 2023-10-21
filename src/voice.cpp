// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "voice.h"

#include "logging.h"

namespace uc {

Voice *Voice::s_instance = nullptr;

Voice::Voice(core::Api *core, const QString &url, QObject *parent) : QObject(parent), m_core(core), m_url(url) {
    Q_ASSERT(s_instance == nullptr);
    s_instance = this;

    QAudioFormat format;
    format.setSampleRate(16000);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setSampleType(QAudioFormat::SignedInt);
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setCodec("audio/pcm");

    QAudioDeviceInfo deviceInfo = QAudioDeviceInfo::defaultInputDevice();

    for (auto &di : QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
        qCDebug(lcVoice()) << "Audio device" << di.deviceName();
        if (!di.deviceName().contains("Microphonetop")) {
            deviceInfo = di;
        }
    }

    qCDebug(lcVoice()) << "Audio device" << deviceInfo.deviceName();

    if (!deviceInfo.isFormatSupported(format)) {
        qCWarning(lcVoice()) << "Default format not supported, using nearest";
        format = deviceInfo.nearestFormat(format);
    }

    m_audioInput = new QAudioInput(deviceInfo, format);
    m_audioInput->setBufferSize(3200 * 8);

    m_workerThread = new QThread(this);

    VoskWorker *voskWorker = new VoskWorker;
    voskWorker->moveToThread(m_workerThread);

    QObject::connect(m_workerThread, &QThread::finished, voskWorker, &QObject::deleteLater);
    QObject::connect(this, &Voice::startWorker, voskWorker, &VoskWorker::startListening);
    QObject::connect(this, &Voice::stopWorker, voskWorker, &VoskWorker::stopListening);
    QObject::connect(voskWorker, &VoskWorker::result, this, &Voice::onResult);
    QObject::connect(voskWorker, &VoskWorker::finalResult, this, &Voice::onFinalResult);

    m_workerThread->start();

    QString first = m_url.split(":")[1];
    m_url = "ws:" + first + ":2700";

    qCDebug(lcVoice()) << first << m_url;

    QObject::connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &Voice::onTextMessageReceived);
    QObject::connect(&m_webSocket, static_cast<void (QWebSocket::*)(QAbstractSocket::SocketError)>(&QWebSocket::error),
                     this, &Voice::onError);
    QObject::connect(&m_webSocket, &QWebSocket::stateChanged, this, &Voice::onStateChanged);

    qmlRegisterSingletonType<Voice>("Voice", 1, 0, "Voice", &Voice::qmlInstance);
}

Voice::~Voice() {
    s_instance = nullptr;
    m_workerThread->quit();
    m_workerThread->wait();
}

void Voice::startListening() {
    qCDebug(lcVoice()) << "Start listening";
    m_webSocket.open(QUrl(m_url));

    auto io = m_audioInput->start();
    //    m_audioInput->setVolume(1.0);

    QObject::connect(io, &QIODevice::readyRead, [=]() {
        if (m_audioInput->state() == QAudio::State::StoppedState) {
            return;
        }

        m_buffer.append(io->readAll());
        m_bufferCount++;

        if (m_bufferCount == 10) {
            emit startWorker(m_buffer.data(), m_buffer.size());

            m_bufferCount = 0;
            m_buffer.clear();
        }
    });
}

void Voice::stopListening() {
    qCDebug(lcVoice()) << "Stop listening";
    QTimer::singleShot(1000, [=] {
        m_audioInput->stop();
        m_buffer.clear();

        emit stopWorker();
    });
}

QObject *Voice::qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(scriptEngine)

    QObject *obj = s_instance;
    engine->setObjectOwnership(obj, QQmlEngine::CppOwnership);

    return obj;
}

void Voice::onTextMessageReceived(const QString &message) {
    // if not a valid socket, do nothing
    if (!m_webSocket.isValid()) {
        qCWarning(lcVoice()) << "Invalid socket, dropping message";
        return;
    }

    //    qCDebug(lcVoice()).noquote() << message.simplified();

    // Parse message to JSON
    QJsonParseError parseerror;
    QJsonDocument   doc = QJsonDocument::fromJson(message.toUtf8(), &parseerror);
    if (parseerror.error != QJsonParseError::NoError) {
        qCCritical(lcVoice()) << "JSON error:" << parseerror.errorString();
        return;
    }

    QVariantMap map = doc.toVariant().toMap();

    if (map.contains("intent")) {
        if (map.value("intent").toMap().value("probability").toFloat() < 0.5) {
            return;
        }

        QString command;

        if (map.contains("intent")) {
            command = map.value("intent").toMap().value("intentName").toString();
            if (command.isEmpty()) {
                if (m_webSocket.isValid()) {
                    m_webSocket.close();
                }
                qCWarning(lcVoice()) << "Command was not recognised";
                emit error(tr("Command was not recognised"));
                return;
            }
        }

        QString  entity;
        QVariant param;

        QVariantList intentSlots = map.value("slots").toList();

        for (QVariantList::iterator i = intentSlots.begin(); i != intentSlots.end(); i++) {
            if (i->toMap().value("slotName").toString().contains("entity")) {
                entity = i->toMap().value("value").toMap().value("value").toString();
            } else if (i->toMap().value("slotName").toString().contains("brightness")) {
                param = i->toMap().value("value").toMap().value("value").toDouble();
            }
        }

        if (entity.isEmpty()) {
            if (m_webSocket.isValid()) {
                m_webSocket.close();
            }
            qCWarning(lcVoice()) << "Entity was not recognised";
            emit error(tr("Entity was not recognised"));
            return;
        }

        emit commandExecuted(command, entity, param);

        if (m_webSocket.isValid()) {
            m_webSocket.close();
        }
    }
}

void Voice::onStateChanged(QAbstractSocket::SocketState state) {
    qCDebug(lcVoice()) << "State:" << state;
}

void Voice::onError(QAbstractSocket::SocketError error) {
    qCWarning(lcVoice()) << "Error: " << error << m_webSocket.errorString();

    if (m_webSocket.isValid()) {
        m_webSocket.close();
    }

    m_webSocket.open(QUrl(m_url));
}

void Voice::onResult(QString message) {
    emit transcriptionUpdated(message);
}

void Voice::onFinalResult(QString message) {
    emit transcriptionUpdated(message);
    qCDebug(lcVoice()) << "Final message" << message;
    m_webSocket.sendTextMessage(message);
}

VoskWorker::VoskWorker(QObject *parent) : QObject(parent) {}

VoskWorker::~VoskWorker() {}

void VoskWorker::startListening(char *buffer, int len) {}

void VoskWorker::stopListening() {}

void VoskWorker::processVoskResult(QString message) {
    QJsonParseError parseerror;
    QJsonDocument   doc = QJsonDocument::fromJson(message.toUtf8(), &parseerror);
    if (parseerror.error != QJsonParseError::NoError) {
        qCCritical(lcVoice()) << "JSON error:" << parseerror.errorString();
        return;
    }

    QVariantMap map = doc.toVariant().toMap();

    if (!map.value("text").toString().isEmpty()) {
        auto str = map.value("text").toString();
        m_lastRecognition = str;

        emit result(str);
        qCDebug(lcVoice()) << "Text:" << str;
    }

    if (!map.value("partial").toString().isEmpty()) {
        auto str = map.value("partial").toString();
        m_lastRecognition = str;

        emit result(str);
        qCDebug(lcVoice()) << "Partial:" << str;
    }
}
}  // namespace uc
