// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "climate.h"

#include "../../logging.h"
#include "../../util.h"

namespace uc {
namespace ui {
namespace entity {

static constexpr const char *UNIT_LABEL_CELSIUS = "°C";
static constexpr const char *UNIT_LABEL_FAHRENHEIT = "°F";

static constexpr float TEMP_STEP_CELSIUS = 0.5;
static constexpr float TEMP_STEP_FAHRENHEIT = 1;
static constexpr float MIN_TEMP_CELSIUS = 10;
static constexpr float MAX_TEMP_CELSIUS = 30;
static constexpr float MIN_TEMP_FAHRENHEIT = 50;
static constexpr float MAX_TEMP_FAHRENHEIT = 86;

Climate::Climate(const QString &id, const QString &name, QVariantMap nameI18n, const QString &icon, const QString &area,
                 const QString &deviceClass, const QStringList &features, bool enabled, QVariantMap attributes,
                 QVariantMap options, const QString &integrationId, const Config::UnitSystems unitSystem,
                 QObject *parent)
    : Base(id, name, nameI18n, icon, area, Type::Climate, enabled, attributes, integrationId, false, parent),
      // set defaults for Celsius
      m_temperatureUnit(TemperatureUnit::CELSIUS),
      m_currentTemperature(0),
      m_targetTemperature(0),
      m_targetTemperatureHigh(MAX_TEMP_CELSIUS),
      m_targetTemperatureLow(MIN_TEMP_CELSIUS),
      m_options(options),
      m_useSystemUnit(true),
      m_temperatureLabel(UNIT_LABEL_CELSIUS),
      m_targetTemperatureStep(TEMP_STEP_CELSIUS) {
    qCDebug(lcClimate()) << "Climate entity constructor";

    updateFeatures<ClimateFeatures::Enum>(features);

    // attributes
    if (attributes.size() > 0) {
        for (QVariantMap::iterator i = attributes.begin(); i != attributes.end(); i++) {
            updateAttribute(uc::Util::FirstToUpper(i.key()), i.value());
        }
    }

    // device class
    int deviceClassEnum = -1;

    if (!deviceClass.isEmpty()) {
        deviceClassEnum = Util::convertStringToEnum<ClimateDeviceClass::Enum>(deviceClass);
    }

    if (deviceClassEnum != -1) {
        m_deviceClass = deviceClass;
    } else {
        m_deviceClass = QVariant::fromValue(ClimateDeviceClass::Climate).toString();
    }

    // options
    if (options.contains("temperature_unit")) {
        bool ok = false;
        auto value = options.value("temperature_unit").toString();
        auto unit = Util::convertStringToEnum<TemperatureUnit::Enum>(value, &ok);
        if (ok) {
            m_temperatureUnit = unit;
            // use specified unit from entity options and ignore system localization
            m_useSystemUnit = false;
        } else {
            qCWarning(lcClimate()) << "Invalid temperature_unit value" << value << "in entity" << m_id
                                   << ". Using CELSIUS as default!";
        }
    } else {
        qCInfo(lcClimate()) << "Climate entity" << m_id
                            << "has no temperature_unit option. Using localization settings.";
        m_temperatureUnit =
            unitSystem == Config::UnitSystems::Metric ? TemperatureUnit::CELSIUS : TemperatureUnit::FAHRENHEIT;
    }

    updateTemperaturUnitValues();

    if (options.contains("fan_modes")) {
        m_fanModes = options.value("fan_modes").toStringList();
    }
}

Climate::~Climate() {
    qCDebug(lcClimate()) << "Climate entity destructor";
}

void Climate::updateTemperaturUnitValues() {
    if (m_temperatureUnit == TemperatureUnit::CELSIUS) {
        m_temperatureLabel = UNIT_LABEL_CELSIUS;
        m_targetTemperatureStep = TEMP_STEP_CELSIUS;
        m_minTemperature = MIN_TEMP_CELSIUS;
        m_maxTemperature = MAX_TEMP_CELSIUS;
    } else {
        m_temperatureLabel = UNIT_LABEL_FAHRENHEIT;
        m_targetTemperatureStep = TEMP_STEP_FAHRENHEIT;
        m_minTemperature = MIN_TEMP_FAHRENHEIT;
        m_maxTemperature = MAX_TEMP_FAHRENHEIT;
    }

    if (m_options.contains("target_temperature_step")) {
        bool ok = false;
        auto value = m_options.value("target_temperature_step");
        auto step = value.toFloat(&ok);
        if (ok && step > 0.09) {
            m_targetTemperatureStep = step;
        } else {
            qCWarning(lcClimate()) << "Invalid target_temperature_step value" << value << "in entity" << m_id
                                   << ". Using default!";
        }
    }

    if (m_options.contains("max_temperature")) {
        bool ok = false;
        auto value = m_options.value("max_temperature");
        auto max = value.toFloat(&ok);
        if (ok) {
            m_maxTemperature = max;
        } else {
            qCWarning(lcClimate()) << "Invalid max_temperature value" << value << "in entity" << m_id
                                   << ". Using default:" << m_maxTemperature;
        }
    } else {
        qCInfo(lcClimate()) << "Climate entity" << m_id
                            << "has no max_temperature option. Using default:" << m_maxTemperature;
    }

    if (m_options.contains("min_temperature")) {
        bool ok = false;
        auto value = m_options.value("min_temperature");
        auto min = value.toFloat(&ok);
        if (ok) {
            m_minTemperature = min;
        } else {
            qCWarning(lcClimate()) << "Invalid m_minTemperature value" << value << "in entity" << m_id
                                   << ". Using default:" << m_minTemperature;
        }
    } else {
        qCInfo(lcClimate()) << "Climate entity" << m_id
                            << "has no m_minTemperature option. Using default:" << m_minTemperature;
    }

    // create temperature range model for ui
    // #279 target temperature step is an option with the smallest step of 0.1
    m_model.clear();
    int max = static_cast<int>(nearbyint(m_maxTemperature * 10));
    int min = static_cast<int>(nearbyint(m_minTemperature * 10));
    int step = static_cast<int>(nearbyint(m_targetTemperatureStep * 10));
    for (int i = max; i >= min; i -= step) {
        if (i % 10) {
            // round to one decimal place
            m_model.append(QString::number(static_cast<float>(i) / 10, 'f', 1));
        } else {
            // don't show decimal place for full numbers
            m_model.append(QString::number(i / 10));
        }
    }
}

void Climate::turnOn() {
    sendCommand(ClimateCommands::On);
}

void Climate::turnOff() {
    sendCommand(ClimateCommands::Off);
}

void Climate::setHvacMode(int mode) {
    QVariantMap params;
    params.insert("hvac_mode", QVariant::fromValue(static_cast<ClimateStates::Enum>(mode)).toString().toUpper());
    sendCommand(ClimateCommands::Hvac_mode, params);
}

void Climate::setTargetTemperature(float temperature) {
    QVariantMap params;
    params.insert("temperature", temperature);
    sendCommand(m_temperatureUnit == TemperatureUnit::CELSIUS ? ClimateCommands::Target_temperature_c
                                                              : ClimateCommands::Target_temperature_f,
                params);
}

void Climate::setTargetTemperatureRange(float low, float high) {
    QVariantMap params;
    params.insert("target_temperature_high", high);
    params.insert("target_temperature_low", low);
    // TODO(#279) verify °C / °F handling. We might need C & F specific commands.
    sendCommand(ClimateCommands::Target_temperature_range, params);
}

void Climate::fanMode(int mode) {
    // TODO(marton): convert mode to enum
    QVariantMap params;
    params.insert("fan_mode", mode);
    sendCommand(ClimateCommands::Fan_mode, params);
}

int Climate::getModelIndexFromTemperature(float temperature) {
    for (int i = 0; i < m_model.length(); i++) {
        if (Util::FloatCompare(m_model[i].toFloat(), temperature)) {
            return i;
        }
    }

    return -1;
}

void Climate::sendCommand(ClimateCommands::Enum cmd, QVariantMap params) {
    Base::sendCommand(QVariant::fromValue(cmd).toString(), params);
}

void Climate::sendCommand(ClimateCommands::Enum cmd) {
    sendCommand(cmd, QVariantMap());
}

bool Climate::updateAttribute(const QString &attribute, QVariant data) {
    bool ok = false;

    // convert to enum
    ClimateAttributes::Enum attributeEnum = Util::convertStringToEnum<ClimateAttributes::Enum>(attribute);

    switch (attributeEnum) {
        case ClimateAttributes::State: {
            int newState = Util::convertStringToEnum<ClimateStates::Enum>(uc::Util::FirstToUpper(data.toString()));

            if (m_state != newState && newState != -1) {
                m_state = newState;
                ok = true;
                emit stateChanged(m_id, m_state);

                m_stateAsString =
                    Util::convertEnumToString<ClimateStates::Enum>(static_cast<ClimateStates::Enum>(m_state));
                emit stateAsStringChanged();

                m_stateInfo1 = getStateAsString();
                emit stateInfoChanged();
            }
            break;
        }
        case ClimateAttributes::Current_temperature: {
            float newTemp = data.toFloat();

            if (!Util::FloatCompare(newTemp, m_currentTemperature)) {
                m_currentTemperature = newTemp;
                ok = true;
                emit currentTemperatureChanged();

                m_stateInfo2 = QString::number(m_currentTemperature) + m_temperatureLabel;
                emit stateInfoChanged();
            }
            break;
        }
        case ClimateAttributes::Target_temperature: {
            float newTemp = data.toFloat();

            if (!Util::FloatCompare(newTemp, m_targetTemperature)) {
                m_targetTemperature = newTemp;
                ok = true;
                emit targetTemperatureChanged();
            }
            break;
        }
        case ClimateAttributes::Target_temperature_high: {
            float newTemp = data.toFloat();

            if (!Util::FloatCompare(newTemp, m_targetTemperatureHigh)) {
                m_targetTemperatureHigh = newTemp;
                ok = true;
                emit targetTemperatureHighChanged();
            }
            break;
        }
        case ClimateAttributes::Target_temperature_low: {
            float newTemp = data.toFloat();

            if (!Util::FloatCompare(newTemp, m_targetTemperatureLow)) {
                m_targetTemperatureLow = newTemp;
                ok = true;
                emit targetTemperatureLowChanged();
            }
            break;
        }
        case ClimateAttributes::Fan_mode: {
            int newMode = data.toInt();

            if (m_fanMode != newMode) {
                m_fanMode = newMode;
                ok = true;
                emit fanModeChanged();
            }
            break;
        }
    }

    return ok;
}

void Climate::onUnitSystemChanged(Config::UnitSystems unitSystem) {
    if (!m_useSystemUnit) {
        return;
    }

    auto unit = unitSystem == Config::UnitSystems::Metric ? TemperatureUnit::CELSIUS : TemperatureUnit::FAHRENHEIT;

    if (unit == m_temperatureUnit) {
        return;
    }
    m_temperatureUnit = unit;

    qCDebug(lcClimate()) << "system unit changed and entity" << m_id << "has no temperature_unit option, switching to"
                         << unit;

    updateTemperaturUnitValues();

    emit targetTemperatureLowChanged();
    emit targetTemperatureHighChanged();

    emit modelChanged();
}

}  // namespace entity
}  // namespace ui
}  // namespace uc
