// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "switch.h"

#include "../../logging.h"
#include "../../util.h"

namespace uc {
namespace ui {
namespace entity {

Switch::Switch(const QString &id, const QString &name, QVariantMap nameI18n, const QString &icon, const QString &area,
               const QString &deviceClass, const QStringList &features, bool enabled, QVariantMap attributes,
               QVariantMap options, const QString &integrationId, QObject *parent)
    : Base(id, name, nameI18n, icon, area, Type::Switch, enabled, attributes, integrationId, false, parent),
      m_readable(true) {
    qCDebug(lcSwitch()) << "Switch entity constructor";

    updateFeatures<SwitchFeatures::Enum>(features);

    // attributes
    if (attributes.size() > 0) {
        for (QVariantMap::iterator i = attributes.begin(); i != attributes.end(); i++) {
            updateAttribute(uc::Util::FirstToUpper(i.key()), i.value());
        }
    }

    // device class
    int deviceClassEnum = -1;

    if (!deviceClass.isEmpty()) {
        deviceClassEnum = Util::convertStringToEnum<SwitchDeviceClass::Enum>(deviceClass);
    }

    if (deviceClassEnum != -1) {
        m_deviceClass = deviceClass;
    } else {
        m_deviceClass = QVariant::fromValue(SwitchDeviceClass::Switch).toString();
    }

    // options
    if (options.contains("readable")) {
        m_readable = options.value("readable").toBool();
    }
}

Switch::~Switch() {
    qCDebug(lcSwitch()) << "Switch entity destructor";
}

void Switch::turnOn() {
    sendCommand(SwitchCommands::On);
}

void Switch::turnOff() {
    sendCommand(SwitchCommands::Off);
}

void Switch::toggle() {
    sendCommand(SwitchCommands::Toggle);
}

void Switch::sendCommand(SwitchCommands::Enum cmd, QVariantMap params) {
    Base::sendCommand(QVariant::fromValue(cmd).toString(), params);
}

void Switch::sendCommand(SwitchCommands::Enum cmd) {
    sendCommand(cmd, QVariantMap());
}

bool Switch::updateAttribute(const QString &attribute, QVariant data) {
    bool ok = false;

    // convert to enum
    SwitchAttributes::Enum attributeEnum = Util::convertStringToEnum<SwitchAttributes::Enum>(attribute);

    switch (attributeEnum) {
        case SwitchAttributes::State: {
            int newState = Util::convertStringToEnum<SwitchStates::Enum>(uc::Util::FirstToUpper(data.toString()));

            if (m_state != newState && newState != -1) {
                m_state = newState;
                ok = true;
                emit stateChanged(m_id, m_state);

                m_stateAsString = SwitchStates::getTranslatedString(static_cast<SwitchStates::Enum>(m_state));
                emit stateAsStringChanged();

                m_stateInfo = getStateAsString();
                emit stateInfoChanged();
            }
            break;
        }
    }

    return ok;
}

void Switch::onLanguageChangedTypeSpecific()
{
    QTimer::singleShot(500, [=]() {
        m_stateAsString = SwitchStates::getTranslatedString(static_cast<SwitchStates::Enum>(m_state));
        emit stateAsStringChanged();

        m_stateInfo = getStateAsString();
        emit stateInfoChanged();
    });
}

}  // namespace entity
}  // namespace ui
}  // namespace uc
