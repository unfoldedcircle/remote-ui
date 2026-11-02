// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "number.h"

#include "../../logging.h"
#include "../../util.h"
#include "valueFormat.h"

namespace uc {
namespace ui {
namespace entity {

// smallest step size of the number entity options
static constexpr double MIN_STEP = 0.000001;

Number::Number(const QString &id, QVariantMap nameI18n, const QString &language, const QString &icon,
               const QString &area, const QString &deviceClass, bool enabled, QVariantMap attributes,
               QVariantMap options, const QString &integrationId, QObject *parent)
    : Base(id, nameI18n, language, icon, area, Type::Number, enabled, attributes, integrationId, false, parent) {
    qCDebug(lcNumber()) << "Number entity constructor" << deviceClass;

    m_numberDeviceClass = NumberDeviceClass::fromString(deviceClass);
    m_deviceClass = NumberDeviceClass::toString(m_numberDeviceClass);

    m_confirmationTimer.setSingleShot(true);
    m_confirmationTimer.setInterval(CONFIRMATION_TIMEOUT_MS);
    // the driver didn't confirm the value set by the user: show the last known value again
    QObject::connect(&m_confirmationTimer, &QTimer::timeout, this, &Number::clearPending);

    // options first: required for value formatting
    applyOptions(options);

    if (attributes.size() > 0) {
        for (QVariantMap::iterator i = attributes.begin(); i != attributes.end(); i++) {
            updateAttribute(uc::Util::FirstToUpper(i.key()), i.value());
        }
    }
}

Number::~Number() {
    qCDebug(lcNumber()) << "Number entity destructor";
}

int Number::getDecimals() {
    if (m_userDecimals >= 0) {
        return m_userDecimals;
    }
    if (m_decimals >= 0) {
        return m_decimals;
    }

    // automatic format: the decimal places of step, min and max
    int decimals = ValueFormat::decimalPlaces(m_step);
    if (m_hasMin) {
        decimals = qMax(decimals, ValueFormat::decimalPlaces(m_min));
    }
    if (m_hasMax) {
        decimals = qMax(decimals, ValueFormat::decimalPlaces(m_max));
    }
    return decimals;
}

bool Number::isSliderRepresentation() {
    // an unbounded number has no slider range
    if (!isBounded()) {
        return false;
    }

    switch (m_hasUserDisplayMode ? m_userDisplayMode : m_displayMode) {
        case NumberDisplayMode::Slider:
            return true;
        case NumberDisplayMode::Stepper:
            return false;
        case NumberDisplayMode::Auto:
        default:
            return (m_max - m_min) / m_step <= AUTO_SLIDER_MAX_POSITIONS;
    }
}

QString Number::formatValue(double value) {
    return ValueFormat::fixed(value, getDecimals());
}

void Number::setValue(double value) {
    value = clamp(value);

    QVariantMap params;
    params.insert("value", value);
    sendCommand(NumberCommands::Set_value, params);

    if (!m_readable) {
        // the driver never reports the value: keep the last sent value
        setKnownValue(value);
        return;
    }

    // show the value of the user until the driver reports one
    m_pendingValue = value;
    m_pending = true;
    m_confirmationTimer.start();
    emit valueChanged();
    emit stateInfoChanged();
}

void Number::increment() {
    sendCommand(NumberCommands::Increment);

    if (!m_readable && m_hasValue) {
        setKnownValue(clamp(m_value + m_step));
    }
}

void Number::decrement() {
    sendCommand(NumberCommands::Decrement);

    if (!m_readable && m_hasValue) {
        setKnownValue(clamp(m_value - m_step));
    }
}

void Number::sendCommand(NumberCommands::Enum cmd, QVariantMap params) {
    Base::sendCommand(QVariant::fromValue(cmd).toString(), params);
}

void Number::sendCommand(NumberCommands::Enum cmd) {
    sendCommand(cmd, QVariantMap());
}

void Number::setKnownValue(double value) {
    const bool wasPending = m_pending;
    m_pending = false;
    m_confirmationTimer.stop();

    const bool changed = !m_hasValue || m_value != value;
    m_value = value;
    m_hasValue = true;

    if (changed || wasPending) {
        emit valueChanged();
        emit stateInfoChanged();
    }
}

void Number::clearPending() {
    m_confirmationTimer.stop();
    if (!m_pending) {
        return;
    }
    m_pending = false;
    emit valueChanged();
    emit stateInfoChanged();
}

double Number::clamp(double value) {
    if (m_hasMin) {
        value = qMax(m_min, value);
    }
    if (m_hasMax) {
        value = qMin(m_max, value);
    }
    return value;
}

QString Number::defaultUnit() {
    switch (m_numberDeviceClass) {
        case NumberDeviceClass::Audio_level:
            return QStringLiteral("dB");
        case NumberDeviceClass::Battery:
        case NumberDeviceClass::Humidity:
            return QStringLiteral("%");
        case NumberDeviceClass::Current:
            return QStringLiteral("A");
        case NumberDeviceClass::Duration:
            return QStringLiteral("s");
        case NumberDeviceClass::Energy:
            return QStringLiteral("kWh");
        case NumberDeviceClass::Illuminance:
            return QStringLiteral("lx");
        case NumberDeviceClass::Power:
            return QStringLiteral("W");
        case NumberDeviceClass::Temperature:
            // no conversion to °F: value, min, max and step are in the unit of the entity
            return QStringLiteral("°C");
        case NumberDeviceClass::Voltage:
            return QStringLiteral("V");
        case NumberDeviceClass::Number:
        default:
            return QString();
    }
}

bool Number::updateAttribute(const QString &attribute, QVariant data) {
    bool ok = false;

    // convert to enum
    NumberAttributes::Enum attributeEnum = Util::convertStringToEnum<NumberAttributes::Enum>(attribute);

    switch (attributeEnum) {
        case NumberAttributes::State: {
            int newState = Util::convertStringToEnum<NumberStates::Enum>(uc::Util::FirstToUpper(data.toString()));

            if (m_state != newState && newState != -1) {
                m_state = newState;
                ok = true;
                emit stateChanged(m_id, m_state);

                m_stateAsString = NumberStates::getTranslatedString(static_cast<NumberStates::Enum>(m_state));
                emit stateAsStringChanged();
                emit stateInfoChanged();
            }
            break;
        }
        case NumberAttributes::Value: {
            if (!data.isValid() || data.isNull()) {
                // unavailable or unknown: no value
                ok = true;
                const bool hadValue = hasValue();
                m_pending = false;
                m_confirmationTimer.stop();
                m_hasValue = false;
                if (hadValue) {
                    emit valueChanged();
                    emit stateInfoChanged();
                }
                break;
            }

            bool         converted = false;
            const double newValue = data.toDouble(&converted);
            if (!converted) {
                qCWarning(lcNumber()) << "Ignoring non-numeric value attribute for" << m_id << data;
                break;
            }
            ok = true;
            // any reported value ends the wait for a confirmation
            setKnownValue(newValue);
            break;
        }
        default:
            break;
    }

    return ok;
}

bool Number::updateOptions(QVariant data) {
    applyOptions(data.toMap());
    return true;
}

void Number::applyOptions(const QVariantMap &options) {
    // the core delivers the complete options: a missing option has its default value
    const QVariant min = options.value("min", 0);
    m_hasMin = !min.isNull();
    m_min = m_hasMin ? min.toDouble() : 0;

    const QVariant max = options.value("max", 100);
    m_hasMax = !max.isNull();
    m_max = m_hasMax ? max.toDouble() : 0;

    const double step = options.value("step", 1).toDouble();
    if (step > 0) {
        m_step = qMax(MIN_STEP, step);
    } else {
        qCWarning(lcNumber()) << "Invalid step for" << m_id << options.value("step");
        m_step = 1;
    }

    m_decimals = ValueFormat::decimalsOption(options.value("decimals"));
    m_unit = options.value("unit").toString();
    m_readable = options.value("readable", true).toBool();
    bool converted = false;
    m_displayMode = Util::convertStringToEnum<NumberDisplayMode::Enum>(
        uc::Util::FirstToUpper(options.value("display_mode").toString()), &converted);
    if (!converted) {
        m_displayMode = NumberDisplayMode::Auto;
    }

    // user overrides
    const QVariantMap user = options.value("user").toMap();
    m_userDecimals = ValueFormat::decimalsOption(user.value("decimals"));
    m_userDisplayMode = Util::convertStringToEnum<NumberDisplayMode::Enum>(
        uc::Util::FirstToUpper(user.value("display_mode").toString()), &m_hasUserDisplayMode);
    if (!m_hasUserDisplayMode) {
        m_userDisplayMode = NumberDisplayMode::Auto;
    }

    if (isBounded() && m_max < m_min) {
        qCWarning(lcNumber()) << "Invalid range for" << m_id << "min:" << m_min << "max:" << m_max;
        m_max = m_min;
    }

    emit optionsChanged();
    emit valueChanged();
    emit stateInfoChanged();
}

QString Number::getStateInfo() {
    if (m_state != NumberStates::Enum::On || !hasValue()) {
        return getStateAsString();
    }

    const QString unit = getUnit();
    return unit.isEmpty() ? getFormattedValue() : getFormattedValue() + " " + unit;
}

void Number::onLanguageChangedTypeSpecific() {
    QTimer::singleShot(500, this, [=]() {
        m_stateAsString = NumberStates::getTranslatedString(static_cast<NumberStates::Enum>(m_state));
        emit stateAsStringChanged();
        emit stateInfoChanged();
    });
}

}  // namespace entity
}  // namespace ui
}  // namespace uc
