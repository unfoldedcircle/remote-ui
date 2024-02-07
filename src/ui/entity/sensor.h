// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

/**
 * @see https://github.com/unfoldedcircle/core-api/blob/main/doc/entities/entity_sensor.md
 */

#include "entity.h"

namespace uc {
namespace ui {
namespace entity {

class SensorAttributes : public QObject {
    Q_GADGET
 public:
    enum Enum { State, Value, Unit };
    Q_ENUM(Enum)
};

class SensorStates : public QObject {
    Q_OBJECT
 public:
    enum Enum { Unavailable = 0, Unknown, On };
    Q_ENUM(Enum)

    static QString getTranslatedString(Enum state) {
        switch (state) {
            case Enum::Unavailable:
                return QCoreApplication::translate("Sensor state", "Unavailable");
            case Enum::Unknown:
                return QCoreApplication::translate("Sensor state", "Unknown");
            case Enum::On:
                return QCoreApplication::translate("Sensor state", "On");
            default:
                return Util::convertEnumToString<Enum>(state);
        }
    }
};

class SensorCommands : public QObject {
    Q_GADGET
 public:
    enum Enum {};
    Q_ENUM(Enum)
};

class SensorDeviceClass : public QObject {
    Q_GADGET
 public:
    enum Enum { Custom, Battery, Current, Energy, Humidity, Power, Temperature, Voltage };
    Q_ENUM(Enum)
};

class Sensor : public Base {
    Q_OBJECT

    Q_PROPERTY(QString value READ getValue NOTIFY valueChanged)
    Q_PROPERTY(QString unit READ getUnit NOTIFY unitChanged)

    // options
    Q_PROPERTY(QString customLabel READ getCustomLabel CONSTANT)
    Q_PROPERTY(QString customUnit READ getCustomUnit CONSTANT)
    Q_PROPERTY(QString nativeUnit READ getNativeUnit CONSTANT)
    Q_PROPERTY(int decimals READ getDecimals CONSTANT)
    Q_PROPERTY(int minValue READ getMinValue CONSTANT)
    Q_PROPERTY(int maxValue READ getMaxValue CONSTANT)

 public:
    explicit Sensor(const QString &id, const QString &name, QVariantMap nameI18n, const QString &icon,
                    const QString &area, const QString &deviceClass, bool enabled, QVariantMap attributes,
                    QVariantMap options, const QString &integrationId, QObject *parent);
    ~Sensor();

    QString getValue() { return m_value.toString(); }
    QString getUnit() { return m_unit; }

    // options
    QString getCustomLabel() { return m_customLabel; }
    QString getCustomUnit() { return m_customUnit; }
    QString getNativeUnit() { return m_nativeUnit; }
    int     getDecimals() { return m_decimals; }
    int     getMinValue() { return m_minValue; }
    int     getMaxValue() { return m_maxValue; }

    bool updateAttribute(const QString &attribute, QVariant data) override;

    QString getStateInfo() override { return m_value.toString() + " " + m_unit; }

    void onLanguageChangedTypeSpecific() override;

 signals:
    void valueChanged();
    void unitChanged();

 private:
    QVariant m_value;
    QString  m_unit;

    // options
    QString m_customLabel;
    QString m_customUnit;
    QString m_nativeUnit;
    int     m_decimals;
    int     m_minValue;
    int     m_maxValue;
};

}  // namespace entity
}  // namespace ui
}  // namespace uc
