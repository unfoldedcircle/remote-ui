// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

/**
 * @see https://github.com/unfoldedcircle/core-api/blob/main/doc/entities/entity_climate.md
 */

#include "../../config/config.h"
#include "entity.h"

namespace uc {
namespace ui {
namespace entity {

class ClimateFeatures : public QObject {
    Q_GADGET
 public:
    enum Enum { On_off, Heat, Cool, Current_temperature, Target_temperature, Target_temperature_range, Fan };
    Q_ENUM(Enum)
};

class ClimateAttributes : public QObject {
    Q_GADGET
 public:
    enum Enum {
        State,
        Current_temperature,
        Target_temperature,
        Target_temperature_high,
        Target_temperature_low,
        Fan_mode
    };
    Q_ENUM(Enum)
};

class ClimateStates : public QObject {
    Q_OBJECT
 public:
    enum Enum { Unavailable = 0, Unknown, Off, Heat, Cool, Heat_cool, Fan, Auto };
    Q_ENUM(Enum)

    static QString getTranslatedString(Enum state) {
        switch (state) {
            case Enum::Unavailable:
                return QCoreApplication::translate("Climate state", "Unavailable");
            case Enum::Unknown:
                return QCoreApplication::translate("Climate state", "Unknown");
            case Enum::Off:
                return QCoreApplication::translate("Climate state", "Off");
            case Enum::Heat:
                return QCoreApplication::translate("Climate state", "Heat");
            case Enum::Cool:
                return QCoreApplication::translate("Climate state", "Cool");
            case Enum::Heat_cool:
                return QCoreApplication::translate("Climate state", "Heat/Cool");
            case Enum::Fan:
                return QCoreApplication::translate("Climate state", "Fan");
            case Enum::Auto:
                return QCoreApplication::translate("Climate state", "Auto");
            default:
                return Util::convertEnumToString<Enum>(state);
        }
    }
};

class ClimateCommands : public QObject {
    Q_GADGET
 public:
    enum Enum { On, Off, Hvac_mode, Target_temperature_c, Target_temperature_f, Target_temperature_range, Fan_mode };
    Q_ENUM(Enum)
};

class ClimateDeviceClass : public QObject {
    Q_GADGET
 public:
    enum Enum { Climate };
    Q_ENUM(Enum)
};

class TemperatureUnit : public QObject {
    Q_GADGET
 public:
    enum Enum { CELSIUS, FAHRENHEIT };
    Q_ENUM(Enum)
};

class Climate : public Base {
    Q_OBJECT

    Q_PROPERTY(float currentTemperature READ getCurrentTemperature NOTIFY currentTemperatureChanged)
    Q_PROPERTY(float targetTemperature READ getTargetTemperature NOTIFY targetTemperatureChanged)
    Q_PROPERTY(float targetTemperatureHigh READ getTargetTemperatureHigh NOTIFY targetTemperatureHighChanged)
    Q_PROPERTY(float targetTemperatureLow READ getTargetTemperatureLow NOTIFY targetTemperatureLowChanged)
    Q_PROPERTY(int fanMode READ getFanMode NOTIFY fanModeChanged)

    // options
    Q_PROPERTY(QString temperatureLabel READ getTemperatureLabel NOTIFY modelChanged)
    Q_PROPERTY(float targetTemperatureStep READ getTargetTemperatureStep NOTIFY modelChanged)
    Q_PROPERTY(float maxTemperature READ getMaxTemperature NOTIFY modelChanged)
    Q_PROPERTY(float minTemperature READ getMinTemperature NOTIFY modelChanged)
    Q_PROPERTY(QStringList fanModes READ getFanModes CONSTANT)

    Q_PROPERTY(QVariantList model READ getModel NOTIFY modelChanged)

 public:
    explicit Climate(const QString &id, const QString &name, QVariantMap nameI18n, const QString &icon,
                     const QString &area, const QString &deviceClass, const QStringList &features, bool enabled,
                     QVariantMap attributes, QVariantMap options, const QString &integrationId,
                     const Config::UnitSystems unitSystem, QObject *parent);
    ~Climate();

    float getCurrentTemperature() { return m_currentTemperature; }
    float getTargetTemperature() { return m_targetTemperature; }
    float getTargetTemperatureHigh() { return m_targetTemperatureHigh; }
    float getTargetTemperatureLow() { return m_targetTemperatureLow; }
    int   getFanMode() { return m_fanMode; }

    // options
    QString     getTemperatureLabel() { return m_temperatureLabel; }
    float       getTargetTemperatureStep() { return m_targetTemperatureStep; }
    float       getMaxTemperature() { return m_maxTemperature; }
    float       getMinTemperature() { return m_minTemperature; }
    QStringList getFanModes() { return m_fanModes; }

    QVariantList getModel() { return m_model; }

    QString getStateInfo() override { return m_stateInfo1 + " " + m_stateInfo2; }

    Q_INVOKABLE void turnOn() override;
    Q_INVOKABLE void turnOff() override;
    Q_INVOKABLE void setHvacMode(int mode);
    Q_INVOKABLE void setTargetTemperature(float temperature);
    Q_INVOKABLE void setTargetTemperatureRange(float low, float high);
    Q_INVOKABLE void fanMode(int mode);

    Q_INVOKABLE int getModelIndexFromTemperature(float temperature);

    void sendCommand(ClimateCommands::Enum cmd, QVariantMap params);
    void sendCommand(ClimateCommands::Enum cmd);
    bool updateAttribute(const QString &attribute, QVariant data) override;

    void onLanguageChangedTypeSpecific() override;

 signals:
    void currentTemperatureChanged();
    void targetTemperatureChanged();
    void targetTemperatureHighChanged();
    void targetTemperatureLowChanged();
    void fanModeChanged();
    void modelChanged();

 public slots:
    void onUnitSystemChanged(Config::UnitSystems unitSystem);

 private:
    void updateTemperaturUnitValues();

 private:
    TemperatureUnit::Enum m_temperatureUnit;

    float m_currentTemperature;
    float m_targetTemperature;
    float m_targetTemperatureHigh;
    float m_targetTemperatureLow;
    int   m_fanMode;

    // TODO(marton) use better names, what is info1 & 2?
    QString m_stateInfo1;
    QString m_stateInfo2;

    // options
    QVariantMap m_options;
    bool        m_useSystemUnit;
    QString     m_temperatureLabel;
    float       m_targetTemperatureStep;
    float       m_maxTemperature;
    float       m_minTemperature;
    QStringList m_fanModes;

    QVariantList m_model;
};

}  // namespace entity
}  // namespace ui
}  // namespace uc
