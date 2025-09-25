// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cover.h"

#include "../../logging.h"
#include "../../util.h"

namespace uc {
namespace ui {
namespace entity {

Cover::Cover(const QString &id, QVariantMap nameI18n, const QString &language, const QString &icon, const QString &area,
             const QString &deviceClass, const QStringList &features, bool enabled, QVariantMap attributes,
             const QString &integrationId, QObject *parent)
    : Base(id, nameI18n, language, icon, area, Type::Cover, enabled, attributes, integrationId, false, parent) {
    qCDebug(lcCover()) << "Cover entity constructor";

    updateFeatures<CoverFeatures::Enum>(features);

    // attributes
    if (attributes.size() > 0) {
        for (QVariantMap::iterator i = attributes.begin(); i != attributes.end(); i++) {
            updateAttribute(uc::Util::FirstToUpper(i.key()), i.value());
        }
    }

    // device class
    int deviceClassEnum = -1;

    if (!deviceClass.isEmpty()) {
        deviceClassEnum = Util::convertStringToEnum<CoverDeviceClass::Enum>(deviceClass);
    }

    if (deviceClassEnum != -1) {
        m_deviceClass = deviceClass;
    } else {
        m_deviceClass = QVariant::fromValue(CoverDeviceClass::Blind).toString();
    }

    // some device classes look the same from UI point of view
    switch (deviceClassEnum) {
        case CoverDeviceClass::Shade:
            m_deviceClass = Util::convertEnumToString(CoverDeviceClass::Blind);
            break;
        case CoverDeviceClass::Door:
        case CoverDeviceClass::Gate:
        case CoverDeviceClass::Window:
            m_deviceClass = Util::convertEnumToString(CoverDeviceClass::Window);
            break;
        default:
            break;
    }
}

Cover::~Cover() {
    qCDebug(lcCover()) << "Cover entity destructor";
}

void Cover::turnOn() {
    open();
}

void Cover::turnOff() {
    close();
}

void Cover::open() {
    sendCommand(CoverCommands::Open);
}

void Cover::close() {
    sendCommand(CoverCommands::Close);
}

void Cover::stop() {
    sendCommand(CoverCommands::Stop);
}

void Cover::setPosition(int position) {
    QVariantMap params;
    params.insert("position", position);
    sendCommand(CoverCommands::Position, params);
}

void Cover::setTilt(int tiltPosition) {
    QVariantMap params;
    params.insert("tilt_position", tiltPosition);
    sendCommand(CoverCommands::Tilt, params);
}

void Cover::tiltUp() {
    sendCommand(CoverCommands::Tilt_up);
}

void Cover::tiltDown() {
    sendCommand(CoverCommands::Tilt_down);
}

void Cover::tiltStop() {
    sendCommand(CoverCommands::Tilt_stop);
}

void Cover::sendCommand(CoverCommands::Enum cmd, QVariantMap params) {
    Base::sendCommand(QVariant::fromValue(cmd).toString(), params);
}

void Cover::sendCommand(CoverCommands::Enum cmd) {
    sendCommand(cmd, QVariantMap());
}

bool Cover::updateAttribute(const QString &attribute, QVariant data) {
    bool ok = false;

    // convert to enum
    CoverAttributes::Enum attributeEnum = Util::convertStringToEnum<CoverAttributes::Enum>(attribute);

    switch (attributeEnum) {
        case CoverAttributes::State: {
            int newState = Util::convertStringToEnum<CoverStates::Enum>(uc::Util::FirstToUpper(data.toString()));

            if (m_state != newState && newState != -1) {
                m_state = newState;
                ok = true;
                emit stateChanged(m_id, m_state);

                m_stateAsString = CoverStates::getTranslatedString(static_cast<CoverStates::Enum>(m_state));
                emit stateAsStringChanged();

                m_stateInfo1 = getStateAsString();
                emit stateInfoChanged();
            }
            break;
        }
        case CoverAttributes::Position: {
            int newPos = data.toInt();

            if (m_position != newPos) {
                m_position = newPos;
                ok = true;
                emit positionChanged();

                m_stateInfo2 = QString::number(m_position) + "%";
            }
            break;
        }
        case CoverAttributes::Tilt_position: {
            int newTiltPos = data.toInt();

            if (m_tiltPosition != newTiltPos) {
                m_tiltPosition = newTiltPos;
                ok = true;
                emit tiltPositionChanged();
            }
            break;
        }
    }

    return ok;
}

void Cover::onLanguageChangedTypeSpecific()
{
    QTimer::singleShot(500, [=]() {
        m_stateAsString = CoverStates::getTranslatedString(static_cast<CoverStates::Enum>(m_state));
        emit stateAsStringChanged();

        m_stateInfo1 = getStateAsString();
        emit stateInfoChanged();
    });
}

}  // namespace entity
}  // namespace ui
}  // namespace uc
