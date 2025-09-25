// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "entity.h"

namespace uc {
namespace ui {
namespace entity {

class RemoteFeatures : public QObject {
    Q_GADGET
 public:
    enum Enum { Send, On_Off };
    Q_ENUM(Enum)
};

class RemoteAttributes : public QObject {
    Q_GADGET
 public:
    enum Enum { State };
    Q_ENUM(Enum)
};

class RemoteStates : public QObject {
    Q_OBJECT
 public:
    enum Enum { Unavailable = 0, Unknown, On, Off };
    Q_ENUM(Enum)

    static QString getTranslatedString(Enum state) {
        switch (state) {
            case Enum::Unavailable:
                return QCoreApplication::translate("Remote state", "Unavailable");
            case Enum::Unknown:
                return QCoreApplication::translate("Remote state", "Unknown");
            case Enum::On:
                return QCoreApplication::translate("Remote state", "On");
            case Enum::Off:
                return QCoreApplication::translate("Remote state", "Off");
            default:
                return Util::convertEnumToString<Enum>(state);
        }
    }
};

class RemoteCommands : public QObject {
    Q_GADGET
 public:
    enum Enum { On, Off };
    Q_ENUM(Enum)
};

class RemoteDeviceClass : public QObject {
    Q_GADGET
 public:
    enum Enum { Remote };
    Q_ENUM(Enum)
};

class Remote : public Base {
    Q_OBJECT

    // options
    Q_PROPERTY(QVariantList buttonMapping READ getButtonMapping NOTIFY buttonMappingChanged)
    Q_PROPERTY(QVariantMap ui READ getUiConfig NOTIFY uiConfigChanged)

 public:
    explicit Remote(const QString &id, QVariantMap nameI18n, const QString &language, const QString &icon,
                    const QString &area, const QString &deviceClass, const QStringList &features, bool enabled,
                    QVariantMap attributes, QVariantMap options, const QString &integrationId, QObject *parent);
    ~Remote();

    // options
    QVariantList getButtonMapping() { return m_buttonMapping; }
    QVariantMap  getUiConfig() { return m_uiConfig; }

    Q_INVOKABLE void turnOn() override;
    Q_INVOKABLE void turnOff() override;
    Q_INVOKABLE void toggle();

    void sendCommand(RemoteCommands::Enum cmd, QVariantMap params);
    void sendCommand(RemoteCommands::Enum cmd);
    bool updateAttribute(const QString &attribute, QVariant data) override;
    bool updateOptions(QVariant data) override;

    void onLanguageChangedTypeSpecific() override;

 signals:
    void buttonMappingChanged();
    void uiConfigChanged();

 private:
    // options
    QVariantList m_buttonMapping;
    QVariantMap  m_uiConfig;
};

}  // namespace entity
}  // namespace ui
}  // namespace uc
