// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJSEngine>
#include <QJsonObject>
#include <QObject>
#include <QQmlEngine>

#include "../util.h"
#include "hardwareModel.h"

namespace uc {
namespace hw {

class Info : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString modelNumber READ getModelNumber CONSTANT)
    Q_PROPERTY(QString serialNumber READ getSerialNumber CONSTANT)
    Q_PROPERTY(QString revision READ getRevision CONSTANT)

 public:
    explicit Info(QObject *parnet = nullptr);
    ~Info();

    // Q_PROPERTY methods
    QString getModelNumber() { return m_modelNumber; }
    QString getSerialNumber() { return m_serialNumber; }
    QString getRevision() { return m_revision; }

    void set(HardwareModel::Enum modelNumber, const QString &serialNumber, const QString &revision);

    static QObject *qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine);

 private:
    static Info *s_instance;

    QString m_modelNumber;
    QString m_serialNumber;
    QString m_revision;
};

}  // namespace hw
}  // namespace uc
