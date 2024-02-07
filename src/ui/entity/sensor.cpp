// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sensor.h"

#include "../../logging.h"
#include "../../util.h"

namespace uc {
namespace ui {
namespace entity {

Sensor::Sensor(const QString &id, const QString &name, QVariantMap nameI18n, const QString &icon, const QString &area,
               const QString &deviceClass, bool enabled, QVariantMap attributes, QVariantMap options,
               const QString &integrationId, QObject *parent)
    : Base(id, name, nameI18n, icon, area, Type::Sensor, enabled, attributes, integrationId, false, parent),
      m_customLabel(QString()),
      m_customUnit(QString()),
      m_nativeUnit(QString()),
      m_decimals(0) {
    qCDebug(lcSensor()) << "Sensor entity constructor";

    // attributes
    if (attributes.size() > 0) {
        for (QVariantMap::iterator i = attributes.begin(); i != attributes.end(); i++) {
            updateAttribute(uc::Util::FirstToUpper(i.key()), i.value());
        }
    }

    // device class
    int deviceClassEnum = -1;

    if (!deviceClass.isEmpty()) {
        deviceClassEnum = Util::convertStringToEnum<SensorDeviceClass::Enum>(deviceClass);
    }

    if (deviceClassEnum != -1) {
        m_deviceClass = deviceClass;
    } else {
        m_deviceClass = QVariant::fromValue(SensorDeviceClass::Custom).toString();
    }

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
    switch (deviceClassEnum) {
        case SensorDeviceClass::Custom:
            m_unit = "";
            break;
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
    }
}

Sensor::~Sensor() {
    qCDebug(lcSensor()) << "Sensor entity destructor";
}

bool Sensor::updateAttribute(const QString &attribute, QVariant data) {
    bool ok = false;

    // convert to enum
    SensorAttributes::Enum attributeEnum = Util::convertStringToEnum<SensorAttributes::Enum>(attribute);

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
            m_value = data;
            ok = true;
            emit valueChanged();
            emit stateInfoChanged();
            break;
        }
        case SensorAttributes::Unit: {
            m_value = data.toString();
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
