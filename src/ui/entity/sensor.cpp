// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sensor.h"

#include "../../logging.h"
#include "../../util.h"
#include "valueFormat.h"

namespace uc {
namespace ui {
namespace entity {

static bool isNumber(const QVariant& value) {
    switch (static_cast<QMetaType::Type>(value.type())) {
        case QMetaType::Double:
        case QMetaType::Float:
        case QMetaType::Int:
        case QMetaType::UInt:
        case QMetaType::LongLong:
        case QMetaType::ULongLong:
            return true;
        default:
            return false;
    }
}

Sensor::Sensor(const QString& id, QVariantMap nameI18n, const QString& language, const QString& icon,
               const QString& area, const QString& deviceClass, bool enabled, QVariantMap attributes,
               QVariantMap options, const QString& integrationId, QObject* parent)
    : Base(id, nameI18n, language, icon, area, Type::Sensor, enabled, attributes, integrationId, false, parent),
      m_sensorDeviceClass(SensorDeviceClass::Custom),
      m_binarySensorDeviceClass(BinarySensorDeviceClass::None),
      m_customLabel(QString()),
      m_customUnit(QString()),
      m_nativeUnit(QString()),
      m_locale(language),
      m_decimals(-1) {
    qCDebug(lcSensor()) << "Sensor entity constructor";

    // device class
    m_sensorDeviceClass = SensorDeviceClass::fromString(deviceClass);
    m_deviceClass = SensorDeviceClass::toString(m_sensorDeviceClass);

    // options
    if (options.contains("custom_label")) {
        m_customLabel = options.value("custom_label").toString();
    }
    // a timestamp and a date have no unit, and a duration is formatted with the unit it is measured in
    const bool dateTimeClass = m_sensorDeviceClass == SensorDeviceClass::Date ||
                               m_sensorDeviceClass == SensorDeviceClass::Duration ||
                               m_sensorDeviceClass == SensorDeviceClass::Timestamp;
    if (options.contains("custom_unit") && !dateTimeClass) {
        m_customUnit = options.value("custom_unit").toString();
    }
    if (options.contains("native_unit")) {
        m_nativeUnit = options.value("native_unit").toString();
    }
    m_decimals = ValueFormat::decimalsOption(options.value("decimals"));
    m_userDecimals = ValueFormat::decimalsOption(options.value("user").toMap().value("decimals"));
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
        case SensorDeviceClass::Duration:
            m_unit = "s";
            break;
        case SensorDeviceClass::Custom:
        case SensorDeviceClass::Date:
        case SensorDeviceClass::Timestamp:
            // no default unit
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
            // stored as reported, a binary sensor's on/off is translated by getValue()
            m_value = data;
            ok = true;
            emit valueChanged();
            if (m_sensorDeviceClass == SensorDeviceClass::Duration) {
                // the unit is only shown if the value is not shown as a time span
                emit unitChanged();
            }
            emit stateInfoChanged();
            break;
        }
        case SensorAttributes::Unit: {
            if (m_sensorDeviceClass == SensorDeviceClass::Binary) {
                m_binarySensorDeviceClass = BinarySensorDeviceClass::fromString(data.toString());
                // the translated on/off text depends on the device class
                emit valueChanged();
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

QString Sensor::getValue() {
    if (m_sensorDeviceClass == SensorDeviceClass::Binary) {
        const QString value = m_value.toString();
        // getTranslatedValue() reads everything but "on" as off: no value stays no value
        return value.isEmpty() ? value : BinarySensorDeviceClass::getTranslatedValue(m_binarySensorDeviceClass, value);
    }

    // a value that is not in the format of the device class is shown as any other value
    QString formatted;
    switch (m_sensorDeviceClass) {
        case SensorDeviceClass::Date:
            formatted = ValueFormat::date(m_value.toString(), m_locale);
            break;
        case SensorDeviceClass::Duration:
            formatted = timeSpan();
            break;
        case SensorDeviceClass::Timestamp:
            formatted = ValueFormat::dateTime(m_value.toString(), m_timeZone, m_clock24h, m_locale);
            break;
        default:
            break;
    }
    if (!formatted.isEmpty()) {
        return formatted;
    }

    if (isNumber(m_value)) {
        const int decimals = getDecimals();
        return decimals >= 0 ? ValueFormat::fixed(m_value.toDouble(), decimals)
                             : ValueFormat::compact(m_value.toDouble());
    }
    // a text value is shown as reported
    return m_value.toString();
}

QString Sensor::timeSpan() const {
    if (m_sensorDeviceClass != SensorDeviceClass::Duration || !isNumber(m_value)) {
        return QString();
    }
    return ValueFormat::timeSpan(m_value.toDouble(), m_unit, m_userDecimals >= 0 ? m_userDecimals : m_decimals);
}

bool Sensor::updateOptions(QVariant data) {
    // only the display format follows an option update, the other options are read once
    const QVariantMap options = data.toMap();
    m_decimals = ValueFormat::decimalsOption(options.value("decimals"));
    m_userDecimals = ValueFormat::decimalsOption(options.value("user").toMap().value("decimals"));

    emit valueChanged();
    if (m_sensorDeviceClass == SensorDeviceClass::Duration) {
        // the decimals round a duration before it is formatted: it may no longer be shown as a time span
        emit unitChanged();
    }
    emit stateInfoChanged();
    return true;
}

void Sensor::onDateTimeFormatChanged(const QString& language, const QString& timezone, bool clock24h) {
    m_locale = QLocale(language);
    m_timeZone = QTimeZone(timezone.toLatin1());
    m_clock24h = clock24h;

    if (m_sensorDeviceClass == SensorDeviceClass::Date || m_sensorDeviceClass == SensorDeviceClass::Timestamp) {
        emit valueChanged();
        emit stateInfoChanged();
    }
}

void Sensor::onLanguageChangedTypeSpecific()
{
    QTimer::singleShot(500, this, [=]() {
        m_stateAsString = SensorStates::getTranslatedString(static_cast<SensorStates::Enum>(m_state));
        emit stateAsStringChanged();

        // a binary sensor's value is a translated on/off text, a time span has translated unit labels
        if (m_sensorDeviceClass == SensorDeviceClass::Binary || m_sensorDeviceClass == SensorDeviceClass::Duration) {
            emit valueChanged();
            emit stateInfoChanged();
        }
    });
}
}  // namespace entity
}  // namespace ui
}  // namespace uc
