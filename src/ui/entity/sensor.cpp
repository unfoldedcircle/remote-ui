// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sensor.h"

#include "../../logging.h"
#include "../../util.h"

namespace uc {
namespace ui {
namespace entity {

Sensor::Sensor(const QString &id, QVariantMap nameI18n, const QString &language, const QString &icon, const QString &area,
               const QString &deviceClass, bool enabled, QVariantMap attributes, QVariantMap options,
               const QString &integrationId, QObject *parent)
    : Base(id, nameI18n, language, icon, area, Type::Sensor, enabled, attributes, integrationId, false, parent),
      m_sensorDeviceClass(SensorDeviceClass::Custom),
      m_binarySensorDeviceClass(BinarySensorDeviceClass::None),
      m_customLabel(QString()),
      m_customUnit(QString()),
      m_nativeUnit(QString()),
      m_decimals(0) {
    qCDebug(lcSensor()) << "Sensor entity constructor";

    // device class
    m_sensorDeviceClass = SensorDeviceClass::fromString(deviceClass);
    m_deviceClass = SensorDeviceClass::toString(m_sensorDeviceClass);

    // options
    if (options.contains("custom_label")) {
        m_customLabel = options.value("custom_label").toString();
    }
    if (options.contains("custom_unit")) {
        m_customUnit = options.value("custom_unit").toString();
    }
    if (options.contains("native_unit")) {
        m_nativeUnit = options.value("native_unit").toString();
    }
    if (options.contains("decimals")) {
        m_decimals = options.value("decimals").toInt();
    }
    if (options.contains("min_value")) {
        m_minValue = options.value("min_value").toInt();
    }
    if (options.contains("max_value")) {
        m_maxValue = options.value("max_value").toInt();
    }

    // set default unit based on device class
    switch (m_sensorDeviceClass) {
        case SensorDeviceClass::Battery:
            m_unit = "%";
            break;
        case SensorDeviceClass::Current:
            m_unit = "A";
            break;
        case SensorDeviceClass::Energy:
            m_unit = "kWh";
            break;
        case SensorDeviceClass::Humidity:
            m_unit = "%";
            break;
        case SensorDeviceClass::Power:
            m_unit = "W";
            break;
        case SensorDeviceClass::Temperature:
            m_unit = "°C";
            break;
        case SensorDeviceClass::Voltage:
            m_unit = "V";
            break;
        case SensorDeviceClass::Binary: {
            // initialize binary device class from unit attribute
            QVariant data = attributes.value("unit", QVariant::fromValue(QString("None")));
            if (!data.isNull()) {
                updateAttribute("unit", data);
            }
            break;
        }
        case SensorDeviceClass::Custom:
            // handled in QML screen
            break;
    }

    // attributes last: requires m_sensorDeviceClass
    if (attributes.size() > 0) {
        for (QVariantMap::iterator i = attributes.begin(); i != attributes.end(); i++) {
            updateAttribute(i.key(), i.value());
        }
    }
}

Sensor::~Sensor() {
    qCDebug(lcSensor()) << "Sensor entity destructor";
}

bool Sensor::updateAttribute(const QString &attribute, QVariant data) {
    bool ok = false;

    // convert to enum
    bool converted;
    SensorAttributes::Enum attributeEnum = SensorAttributes::fromString(attribute, &converted);
    if (!converted) {
        return false;
    }

    switch (attributeEnum) {
        case SensorAttributes::State: {
            int newState = Util::convertStringToEnum<SensorStates::Enum>(uc::Util::FirstToUpper(data.toString()));

            if (m_state != newState && newState != -1) {
                m_state = newState;
                ok = true;
                emit stateChanged(m_id, m_state);

                m_stateAsString = SensorStates::getTranslatedString(static_cast<SensorStates::Enum>(m_state));
                emit stateAsStringChanged();
            }
            break;
        }
        case SensorAttributes::Value: {
            if (m_sensorDeviceClass == SensorDeviceClass::Binary) {
                m_value = BinarySensorDeviceClass::getTranslatedValue(m_binarySensorDeviceClass, data.toString());
            } else {
                m_value = data;
            }
            ok = true;
            emit valueChanged();
            emit stateInfoChanged();
            break;
        }
        case SensorAttributes::Unit: {
            if (m_sensorDeviceClass == SensorDeviceClass::Binary) {
                m_binarySensorDeviceClass = BinarySensorDeviceClass::fromString(data.toString());
            } else {
                m_unit = data.toString();
            }
            ok = true;
            emit unitChanged();
            emit stateInfoChanged();
            break;
        }
    }

    return ok;
}

void Sensor::onLanguageChangedTypeSpecific()
{
    QTimer::singleShot(500, [=]() {
        m_stateAsString = SensorStates::getTranslatedString(static_cast<SensorStates::Enum>(m_state));
        emit stateAsStringChanged();
    });
}
}  // namespace entity
}  // namespace ui
}  // namespace uc
