// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "button.h"

#include "../../logging.h"
#include "../../util.h"

namespace uc {
namespace ui {
namespace entity {

Button::Button(const QString &id, const QString &name, QVariantMap nameI18n, const QString &icon, const QString &area,
               const QString &deviceClass, const QStringList &features, bool enabled, QVariantMap attributes,
               const QString &integrationId, QObject *parent)
    : Base(id, name, nameI18n, icon, area, Type::Button, enabled, attributes, integrationId, false, parent) {
    qCDebug(lcButton()) << "Button entity constructor";

    updateFeatures<ButtonFeatures::Enum>(features);

    if (attributes.size() > 0) {
        for (QVariantMap::iterator i = attributes.begin(); i != attributes.end(); i++) {
            updateAttribute(uc::Util::FirstToUpper(i.key()), i.value());
        }
    }

    // device class
    int deviceClassEnum = -1;

    if (!deviceClass.isEmpty()) {
        deviceClassEnum = Util::convertStringToEnum<ButtonDeviceClass::Enum>(deviceClass);
    }

    if (deviceClassEnum != -1) {
        m_deviceClass = deviceClass;
    } else {
        m_deviceClass = QVariant::fromValue(ButtonDeviceClass::Button).toString();
    }
}

Button::~Button() {
    qCDebug(lcButton()) << "Button entity destructor";
}

void Button::turnOn() {
    push();
}

void Button::push() {
    sendCommand(ButtonCommands::Push);
}

void Button::sendCommand(ButtonCommands::Enum cmd, QVariantMap params) {
    Base::sendCommand(QVariant::fromValue(cmd).toString(), params);
}

void Button::sendCommand(ButtonCommands::Enum cmd) {
    sendCommand(cmd, QVariantMap());
}

bool Button::updateAttribute(const QString &attribute, QVariant data) {
    bool ok = false;

    // convert to enum
    ButtonAttributes::Enum attributeEnum = Util::convertStringToEnum<ButtonAttributes::Enum>(attribute);

    switch (attributeEnum) {
        case ButtonAttributes::State: {
            int newState = Util::convertStringToEnum<ButtonStates::Enum>(uc::Util::FirstToUpper(data.toString()));

            if (m_state != newState && newState != -1) {
                m_state = newState;
                ok = true;
                emit stateChanged(m_id, m_state);

                m_stateAsString =
                    Util::convertEnumToString<ButtonStates::Enum>(static_cast<ButtonStates::Enum>(m_state));
                emit stateAsStringChanged();

                m_stateInfo = getStateAsString();
                emit stateInfoChanged();
            }
            break;
        }
    }

    return ok;
}

}  // namespace entity
}  // namespace ui
}  // namespace uc
