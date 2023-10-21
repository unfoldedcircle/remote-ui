// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

#include "../../logging.h"

namespace uc {
namespace ui {

class Profile : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString id READ getId CONSTANT)
    Q_PROPERTY(QString name READ getName NOTIFY nameChanged)
    Q_PROPERTY(bool restricted READ restricted NOTIFY restrictedChanged)
    Q_PROPERTY(QString icon READ getIcon NOTIFY iconChanged)

 public:
    explicit Profile(const QString &profileId, const QString &name, bool restricted, const QString &icon = QString(),
                     QObject *parent = nullptr)
        : QObject(parent), m_profileId(profileId), m_name(name), m_restricted(restricted), m_icon(icon) {}
    ~Profile() { qCDebug(lcUi()).noquote() << "Profile destroyed:" << m_profileId << m_name; }

    QString getId() { return m_profileId; }
    QString getName() { return m_name; }
    bool    restricted() { return m_restricted; }
    QString getIcon() { return m_icon; }

    void setId(const QString &id) { m_profileId = id; }
    void setName(const QString &name) {
        m_name = name;
        emit nameChanged();
    }
    void setRestricted(bool value) {
        m_restricted = value;
        emit restrictedChanged();
    }
    void setIcon(const QString &icon) {
        m_icon = icon;
        emit iconChanged();
    }

 signals:
    void nameChanged();
    void restrictedChanged();
    void iconChanged();

 private:
    QString m_profileId;
    QString m_name;
    bool    m_restricted;
    QString m_icon;
};

}  // namespace ui
}  // namespace uc
