// Copyright (c) 2022-2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

/**
 * @see https://github.com/unfoldedcircle/core-api/blob/main/doc/entities/entity_switch.md
 */

#include "entity.h"

namespace uc {
namespace ui {
namespace entity {

class SelectAttributes : public QObject {
    Q_GADGET
 public:
    enum Enum { State, Current_option, Options };
    Q_ENUM(Enum)
};

class SelectStates : public QObject {
    Q_OBJECT
 public:
    enum Enum { Unavailable = 0, Unknown, On };
    Q_ENUM(Enum)

    static QString getTranslatedString(Enum state) {
        switch (state) {
            case Enum::Unavailable:
                return QCoreApplication::translate("Select state", "Unavailable");
            case Enum::Unknown:
                return QCoreApplication::translate("Select state", "Unknown");
            case Enum::On:
                return QCoreApplication::translate("Select state", "On");
            default:
                return Util::convertEnumToString<Enum>(state);
        }
    }
};

class SelectCommands : public QObject {
    Q_GADGET
 public:
    enum Enum { Select_option, Select_first, Select_last, Select_next, Select_previous };
    Q_ENUM(Enum)
};

class SelectDeviceClass : public QObject {
    Q_GADGET
 public:
    enum Enum { Select };
    Q_ENUM(Enum)
};

class Select : public Base {
    Q_OBJECT

    Q_PROPERTY(QString currentOption READ getCurrentOption NOTIFY currentOptionChanged)
    Q_PROPERTY(QStringList options READ getOptions NOTIFY optionsChanged)

 public:
    explicit Select(const QString &id, QVariantMap nameI18n, const QString &language, const QString &icon,
                    const QString &area, const QString &deviceClass, bool enabled,
                    QVariantMap attributes, const QString &integrationId, QObject *parent);
    ~Select();

    QString getCurrentOption() { return m_currentOption; }
    QStringList getOptions() { return m_options; }

    Q_INVOKABLE void selectOption(const QString &option);
    Q_INVOKABLE void selectFirst();
    Q_INVOKABLE void selectLast();
    Q_INVOKABLE void selectNext();
    Q_INVOKABLE void selectPrevious();

    void sendCommand(SelectCommands::Enum cmd, QVariantMap params);
    void sendCommand(SelectCommands::Enum cmd);
    bool updateAttribute(const QString &attribute, QVariant data) override;

    void onLanguageChangedTypeSpecific() override;

 signals:
    void currentOptionChanged();
    void optionsChanged();

 private:
    QString m_currentOption;
    QStringList m_options;
};

}  // namespace entity
}  // namespace ui
}  // namespace uc
