// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <qendian.h>
#include <stdio.h>

#include <QAudioDeviceInfo>
#include <QAudioInput>
#include <QBuffer>
#include <QByteArray>
#include <QCoreApplication>
#include <QJSEngine>
#include <QJsonDocument>
#include <QObject>
#include <QQmlEngine>
#include <QThread>
#include <QWebSocket>

#include "core/core.h"

namespace uc {

class VoskWorker : public QObject {
    Q_OBJECT

 public:
    explicit VoskWorker(QObject* parent = nullptr);
    ~VoskWorker();

 public slots:
    void startListening(char* buffer, int len);
    void stopListening();

 signals:
    void result(QString text);
    void finalResult(QString text);

 private:
    void processVoskResult(QString message);

    QString m_lastRecognition;
};

class Voice : public QObject {
    Q_OBJECT

 public:
    explicit Voice(core::Api* core, const QString& url, QObject* parent = nullptr);
    ~Voice();

    Q_INVOKABLE void startListening();
    Q_INVOKABLE void stopListening();

    static QObject* qmlInstance(QQmlEngine* engine, QJSEngine* scriptEngine);

 signals:
    void transcriptionUpdated(QString text);
    void commandExecuted(QString command, QString entity, QVariant param);
    void error(QString message);

    void startWorker(char* buffer, int len);
    void stopWorker();

 private slots:
    void onTextMessageReceived(const QString& message);
    void onStateChanged(QAbstractSocket::SocketState state);
    void onError(QAbstractSocket::SocketError error);
    void onResult(QString message);
    void onFinalResult(QString message);

 private:
    static Voice* s_instance;

    core::Api* m_core;

    QWebSocket m_webSocket;
    QString    m_url;

    QAudioInput* m_audioInput;

    QThread* m_workerThread;

    QByteArray m_buffer;
    int        m_bufferCount = 0;
};
}  // namespace uc
