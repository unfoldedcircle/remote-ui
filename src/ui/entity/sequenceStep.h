// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

namespace uc {
namespace ui {
namespace entity {

class SequenceStep : public QObject {
    Q_OBJECT

    Q_PROPERTY(Type type READ getType NOTIFY typeChanged)
    Q_PROPERTY(int index READ getCurrentIndex NOTIFY currentChanged)
    Q_PROPERTY(int delay READ getDelay NOTIFY delayChanged)
    Q_PROPERTY(QString entityId READ getEntityId NOTIFY entityIdChanged)
    Q_PROPERTY(QString commandId READ getCommandId NOTIFY commandIdChanged)
    Q_PROPERTY(QString error READ getError NOTIFY errorChanged)

 public:
    explicit SequenceStep(QObject *parent) : QObject(parent) {}
    ~SequenceStep() {}

    enum Type { Command, Delay };
    Q_ENUM(Type)

    Type getType() { return m_type; }
    void setType(Type type) {
        m_type = type;
        emit typeChanged();
    }

    int  getCurrentIndex() { return m_currentIndex; }
    void setCurrentIndex(int current) {
        m_currentIndex = current;
        emit currentChanged();
    }

    int  getDelay() { return m_delay; }
    void setDelay(int delay) {
        m_delay = delay;
        emit delayChanged();
    }

    QString getEntityId() { return m_entityId; }
    void    setEntityId(const QString &entityId) {
        m_entityId = entityId;
        emit entityIdChanged();
    }

    QString getCommandId() { return m_commandId; }
    void    setCommandId(const QString &commandId) {
        m_commandId = commandId;
        emit commandIdChanged();
    }

    QString getError() { return m_error; }
    void    setError(const QString &error) {
        m_error = error;
        emit errorChanged();
    }

 signals:
    void typeChanged();
    void currentChanged();
    void delayChanged();
    void entityIdChanged();
    void commandIdChanged();
    void errorChanged();

 private:
    Type    m_type;
    int     m_currentIndex = 0;
    int     m_delay = 0;
    QString m_entityId;
    QString m_commandId;
    QString m_error;
};

}  // namespace entity
}  // namespace ui
}  // namespace uc
