// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

/**
 * @see https://github.com/unfoldedcircle/core-api/blob/main/doc/entities/entity_button.md
 */

#include "entity.h"

namespace uc {
namespace ui {
namespace entity {

class ButtonFeatures : public QObject {
    Q_GADGET
 public:
    enum Enum { Press };
    Q_ENUM(Enum)
};

class ButtonAttributes : public QObject {
    Q_GADGET
 public:
    enum Enum { State };
    Q_ENUM(Enum)
};

class ButtonStates : public QObject {
    Q_OBJECT
 public:
    enum Enum { Unavailable = 0, Unknown, Available, On };
    Q_ENUM(Enum)

    static QString getTranslatedString(Enum state) {
        switch (state) {
            case Enum::Unavailable:
                return QCoreApplication::translate("Button state", "Unavailable");
            case Enum::Unknown:
                return QCoreApplication::translate("Button state", "Unknown");
            case Enum::Available:
                return QCoreApplication::translate("Button state", "Available");
            case Enum::On:
                return QCoreApplication::translate("Button state", "On");
            default:
                return Util::convertEnumToString<Enum>(state);
        }
    }
};

class ButtonCommands : public QObject {
    Q_GADGET
 public:
    enum Enum { Push };
    Q_ENUM(Enum)
};

class ButtonDeviceClass : public QObject {
    Q_GADGET
 public:
    enum Enum { Button };
    Q_ENUM(Enum)
};

class Button : public Base {
    Q_OBJECT

 public:
    explicit Button(const QString &id, QVariantMap nameI18n, const QString &language, const QString &icon,
                    const QString &area, const QString &deviceClass, const QStringList &features, bool enabled,
                    QVariantMap attributes, const QString &integrationId, QObject *parent);
    ~Button();

    Q_INVOKABLE void turnOn() override;
    Q_INVOKABLE void push();

    void sendCommand(ButtonCommands::Enum cmd, QVariantMap params);
    void sendCommand(ButtonCommands::Enum cmd);
    bool updateAttribute(const QString &attribute, QVariant data) override;

    void onLanguageChangedTypeSpecific() override;
};

}  // namespace entity
}  // namespace ui
}  // namespace uc
