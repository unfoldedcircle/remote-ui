// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

/**
 * @see https://github.com/unfoldedcircle/core-api/blob/main/doc/entities/entity_switch.md
 */

#include "entity.h"

namespace uc {
namespace ui {
namespace entity {

class SwitchFeatures : public QObject {
    Q_GADGET
 public:
    enum Enum { On_off, Toggle };
    Q_ENUM(Enum)
};

class SwitchAttributes : public QObject {
    Q_GADGET
 public:
    enum Enum { State };
    Q_ENUM(Enum)
};

class SwitchStates : public QObject {
    Q_OBJECT
 public:
    enum Enum { Unavailable = 0, Unknown, On, Off };
    Q_ENUM(Enum)

    static QString getTranslatedString(Enum state) {
        switch (state) {
            case Enum::Unavailable:
                return QCoreApplication::translate("Switch state", "Unavailable");
            case Enum::Unknown:
                return QCoreApplication::translate("Switch state", "Unknown");
            case Enum::On:
                return QCoreApplication::translate("Switch state", "On");
            case Enum::Off:
                return QCoreApplication::translate("Switch state", "Off");
            default:
                return Util::convertEnumToString<Enum>(state);
        }
    }
};

class SwitchCommands : public QObject {
    Q_GADGET
 public:
    enum Enum { On, Off, Toggle };
    Q_ENUM(Enum)
};

class SwitchDeviceClass : public QObject {
    Q_GADGET
 public:
    enum Enum { Switch, Outlet };
    Q_ENUM(Enum)
};

class Switch : public Base {
    Q_OBJECT

 public:
    explicit Switch(const QString &id, const QString &name, QVariantMap nameI18n, const QString &icon,
                    const QString &area, const QString &deviceClass, const QStringList &features, bool enabled,
                    QVariantMap attributes, QVariantMap options, const QString &integrationId, QObject *parent);
    ~Switch();

    Q_INVOKABLE void turnOn() override;
    Q_INVOKABLE void turnOff() override;
    Q_INVOKABLE void toggle();

    void sendCommand(SwitchCommands::Enum cmd, QVariantMap params);
    void sendCommand(SwitchCommands::Enum cmd);
    bool updateAttribute(const QString &attribute, QVariant data) override;

    void onLanguageChangedTypeSpecific() override;

    // Options
 private:
    bool m_readable;
};

}  // namespace entity
}  // namespace ui
}  // namespace uc
