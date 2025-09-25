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

    /**
     * @brief Convert a string to SensorAttributes:Enum
     * @param key string representation of the enum
     * @param ok out parameter for conversion flag
     * @return the converted enum type. Attention: the ok flag MUST be checked, the enum might be invalid.
     */
    static Enum fromString(const QString& key, bool *ok) {
        return static_cast<Enum>(QMetaEnum::fromType<Enum>().keyToValue(uc::Util::FirstToUpper(key).toUtf8(), ok));
    }
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
    // Supported sensor device classes.
    // WARNING: changing device classes requires QML screen support! Dynamically loaded in components/entities/Base.qml
    enum Enum { Custom, Battery, Current, Energy, Humidity, Power, Temperature, Voltage, Binary };
    Q_ENUM(Enum)

    /**
     * @brief Convert a string to SensorDeviceClass:Enum
     * @param key string representation of the enum
     * @return the converted enum type, or Enum::Custom if no match
     */
    static Enum fromString(const QString& key) {
        bool ok;
        Enum res = static_cast<Enum>(QMetaEnum::fromType<Enum>().keyToValue(uc::Util::FirstToUpper(key).toUtf8(), &ok));
        if (ok) {
            return res;
        }
        return Enum::Custom;
    }

    static QString toString(Enum value) { return QVariant::fromValue(value).toString(); }
};

class BinarySensorDeviceClass : public QObject {
    Q_GADGET
 public:
    enum Enum {
        None,
        Battery,
        Battery_charging,
        Carbon_monoxide,
        Cold,
        Connectivity,
        Door,
        Garage_coor,
        Gas,
        Heat,
        Light,
        Lock,
        Moisture,
        Motion,
        Moving,
        Occupancy,
        Opening,
        Plug,
        Power,
        Presence,
        Problem,
        Running,
        Safety,
        Smoke,
        Sound,
        Tamper,
        Update,
        Vibration,
        Window
    };
    Q_ENUM(Enum)

    /**
     * @brief Convert a string to BinarySensorDeviceClass:Enum
     * @param key string representation of the enum in lower_comel_case notation.
     * @return the converted enum type, or Enum::None if no match
     */
    static Enum fromString(const QString& key) {
        bool ok;
        Enum res = static_cast<Enum>(QMetaEnum::fromType<Enum>().keyToValue(uc::Util::FirstToUpper(key).toUtf8(), &ok));
        if (ok) {
            return res;
        }
        return Enum::None;
    }

    static QString toString(Enum value) { return QVariant::fromValue(value).toString(); }

    static QString getTranslatedValue(Enum deviceClass, const QString &value) {
        if (value.toLower() == "on") {
            switch (deviceClass) {
                case Enum::Battery:
                    return QCoreApplication::translate("Binary sensor state battery", "Normal");
                case Enum::Battery_charging:
                    return QCoreApplication::translate("Binary sensor state battery_charging", "Charging");
                case Enum::Cold:
                    return QCoreApplication::translate("Binary sensor state cold", "Cold");
                case Enum::Connectivity:
                    return QCoreApplication::translate("Binary sensor state connectivity", "Connected");
                case Enum::Door:
                case Enum::Garage_coor:
                    return QCoreApplication::translate("Binary sensor state door", "Opened");
                case Enum::Carbon_monoxide:
                case Enum::Gas:
                    return QCoreApplication::translate("Binary sensor state gas", "Detected");
                case Enum::Heat:
                    return QCoreApplication::translate("Binary sensor state heat", "Hot");
                case Enum::Light:
                    return QCoreApplication::translate("Binary sensor state light", "Light detected");
                case Enum::Lock:
                    return QCoreApplication::translate("Binary sensor state lock", "Unlocked");
                case Enum::Moisture:
                    return QCoreApplication::translate("Binary sensor state moisture", "Wet");
                case Enum::Motion:
                    return QCoreApplication::translate("Binary sensor state motion", "Detected");
                case Enum::Moving:
                    return QCoreApplication::translate("Binary sensor state moving", "Moving");
                case Enum::Occupancy:
                    return QCoreApplication::translate("Binary sensor state occupancy", "Detected");
                case Enum::Opening:
                    return QCoreApplication::translate("Binary sensor state opening", "Open");
                case Enum::Plug:
                    return QCoreApplication::translate("Binary sensor state plug", "Plugged in");
                case Enum::Power:
                    return QCoreApplication::translate("Binary sensor state power", "On");
                case Enum::Presence:
                    return QCoreApplication::translate("Binary sensor state presence", "Home");
                case Enum::Problem:
                    return QCoreApplication::translate("Binary sensor state problem", "Problem");
                case Enum::Running:
                    return QCoreApplication::translate("Binary sensor state running", "Running");
                case Enum::Safety:
                    return QCoreApplication::translate("Binary sensor state safety", "Unsafe");
                case Enum::Smoke:
                    return QCoreApplication::translate("Binary sensor state smoke", "Detected");
                case Enum::Sound:
                    return QCoreApplication::translate("Binary sensor state sound", "Detected");
                case Enum::Tamper:
                    return QCoreApplication::translate("Binary sensor state tamper", "Tampering detected");
                case Enum::Update:
                    return QCoreApplication::translate("Binary sensor state update", "Update detected");
                case Enum::Vibration:
                    return QCoreApplication::translate("Binary sensor state vibration", "Detected");
                case Enum::Window:
                    return QCoreApplication::translate("Binary sensor state window", "Open");
                default:
                    return QCoreApplication::translate("Binary sensor state without device class", "On");
            }
        } else {
            switch (deviceClass) {
                case Enum::Battery:
                    return QCoreApplication::translate("Binary sensor state battery", "Low");
                case Enum::Battery_charging:
                    return QCoreApplication::translate("Binary sensor state battery_charging", "Not charging");
                case Enum::Cold:
                    return QCoreApplication::translate("Binary sensor state cold", "Normal");
                case Enum::Connectivity:
                    return QCoreApplication::translate("Binary sensor state connectivity", "Disconnected");
                case Enum::Door:
                case Enum::Garage_coor:
                    return QCoreApplication::translate("Binary sensor state door", "Closed");
                case Enum::Carbon_monoxide:
                case Enum::Gas:
                    return QCoreApplication::translate("Binary sensor state gas", "Clear");
                case Enum::Heat:
                    return QCoreApplication::translate("Binary sensor state heat", "Normal");
                case Enum::Light:
                    return QCoreApplication::translate("Binary sensor state light", "No light");
                case Enum::Lock:
                    return QCoreApplication::translate("Binary sensor state lock", "Locked");
                case Enum::Moisture:
                    return QCoreApplication::translate("Binary sensor state moisture", "Dry");
                case Enum::Motion:
                    return QCoreApplication::translate("Binary sensor state motion", "Clear");
                case Enum::Moving:
                    return QCoreApplication::translate("Binary sensor state moving", "Not moving");
                case Enum::Occupancy:
                    return QCoreApplication::translate("Binary sensor state occupancy", "Clear");
                case Enum::Opening:
                    return QCoreApplication::translate("Binary sensor state opening", "Closed");
                case Enum::Plug:
                    return QCoreApplication::translate("Binary sensor state plug", "Unplugged");
                case Enum::Power:
                    return QCoreApplication::translate("Binary sensor state power", "Off");
                case Enum::Presence:
                    return QCoreApplication::translate("Binary sensor state presence", "Not home");
                case Enum::Problem:
                    return QCoreApplication::translate("Binary sensor state problem", "Ok");
                case Enum::Running:
                    return QCoreApplication::translate("Binary sensor state running", "Not running");
                case Enum::Safety:
                    return QCoreApplication::translate("Binary sensor state safety", "Safe");
                case Enum::Smoke:
                    return QCoreApplication::translate("Binary sensor state smoke", "Clear");
                case Enum::Sound:
                    return QCoreApplication::translate("Binary sensor state sound", "Clear");
                case Enum::Tamper:
                    return QCoreApplication::translate("Binary sensor state tamper", "Clear");
                case Enum::Update:
                    return QCoreApplication::translate("Binary sensor state update", "Up-to-date");
                case Enum::Vibration:
                    return QCoreApplication::translate("Binary sensor state vibration", "Clear");
                case Enum::Window:
                    return QCoreApplication::translate("Binary sensor state window", "Closed");
                default:
                    return QCoreApplication::translate("Binary sensor state without device class", "Off");
            }
        }
    }
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
    explicit Sensor(const QString &id, QVariantMap nameI18n, const QString &language, const QString &icon,
                    const QString &area, const QString &deviceClass, bool enabled, QVariantMap attributes,
                    QVariantMap options, const QString &integrationId, QObject *parent);
    ~Sensor();

    QString getValue() {
        return m_value.toString();
    }
    QString getUnit() { return m_unit; }

    // options
    QString getCustomLabel() { return m_customLabel; }
    QString getCustomUnit() { return m_customUnit; }
    QString getNativeUnit() { return m_nativeUnit; }
    int     getDecimals() { return m_decimals; }
    int     getMinValue() { return m_minValue; }
    int     getMaxValue() { return m_maxValue; }

    bool updateAttribute(const QString &attribute, QVariant data) override;

    QString getStateInfo() override {
        if (m_sensorDeviceClass == SensorDeviceClass::Binary) {
            return m_value.toString();
        } else {
            return m_value.toString() + " " + m_unit;
        }
    }

    void onLanguageChangedTypeSpecific() override;

 signals:
    void valueChanged();
    void unitChanged();

 private:
    QVariant m_value;
    QString  m_unit;

    SensorDeviceClass::Enum       m_sensorDeviceClass;
    BinarySensorDeviceClass::Enum m_binarySensorDeviceClass;

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
