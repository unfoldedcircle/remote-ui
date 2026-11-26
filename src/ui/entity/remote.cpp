// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "remote.h"

#include "../../logging.h"
#include "../../util.h"

namespace uc {
namespace ui {
namespace entity {

Remote::Remote(const QString &id, QVariantMap nameI18n, const QString &language, const QString &icon, const QString &area,
               const QString &deviceClass, const QStringList &features, bool enabled, QVariantMap attributes,
               QVariantMap options, const QString &integrationId, QObject *parent)
    : Base(id,  nameI18n, language, icon, area, Type::Remote, enabled, attributes, integrationId, false, parent) {
    qCDebug(lcRemote()) << "Remote entity constructor";

    updateFeatures<RemoteFeatures::Enum>(features);

    // attributes
    if (attributes.size() > 0) {
        for (QVariantMap::iterator i = attributes.begin(); i != attributes.end(); i++) {
            updateAttribute(uc::Util::FirstToUpper(i.key()), i.value());
        }
    }

    // device class
    int deviceClassEnum = -1;

    if (!deviceClass.isEmpty()) {
        deviceClassEnum = Util::convertStringToEnum<RemoteDeviceClass::Enum>(deviceClass);
    }

    if (deviceClassEnum != -1) {
        m_deviceClass = deviceClass;
    } else {
        m_deviceClass = QVariant::fromValue(RemoteDeviceClass::Remote).toString();
    }

    // options
    if (options.contains("user_interface")) {
        m_uiConfig = options.value("user_interface").toMap();
    }

    if (options.contains("button_mapping")) {
        m_buttonMapping = options.value("button_mapping").toList();
    }
}

Remote::~Remote() { qCDebug(lcRemote()) << "Remote entity destructor"; }

void Remote::turnOn() { sendCommand(RemoteCommands::On); }

void Remote::turnOff() { sendCommand(RemoteCommands::Off); }

void Remote::toggle() {
    if (m_state == RemoteStates::On) {
        turnOff();
    } else {
        turnOn();
    }
}

void Remote::mappedButtonPressed(const QString &button, const QString &cmdId, const QVariantMap &params,
                                 bool hasLongPress) {
    if (isIrTapOrHold(cmdId, hasLongPress)) {
        // also drops the held state a lost release left behind, so this press is a tap or a hold again
        m_heldButtons.remove(button);
        qCDebug(lcRemote()) << "IR command waits for the release or the first auto-repeat:" << button << cmdId;
        return;
    }

    emit command(m_id, cmdId, params);
}

void Remote::mappedButtonRepeated(const QString &button, const QString &cmdId, const QVariantMap &params,
                                  bool sendPress) {
    if (!isSimpleCommand(cmdId)) {
        emit command(m_id, cmdId, params);
        return;
    }

    m_heldButtons.insert(button);

    QVariantMap repeatParams;
    repeatParams.insert(QStringLiteral("command"), cmdId);
    repeatParams.insert(QStringLiteral("repeat"), kHoldRepeatCount);
    if (sendPress) {
        repeatParams.insert(QStringLiteral("press"), true);
    }

    emit command(m_id, QStringLiteral("remote.send"), repeatParams);
}

void Remote::mappedButtonReleased(const QString &button, const QString &cmdId, const QVariantMap &params,
                                  bool hasLongPress) {
    const bool held = m_heldButtons.remove(button);

    if (isIrTapOrHold(cmdId, hasLongPress) && !held) {
        // released before the first auto-repeat: a tap
        emit command(m_id, cmdId, params);
        return;
    }

    if (isSimpleCommand(cmdId)) {
        emit command(m_id, QStringLiteral("remote.stop_send"), QVariantMap());
    }
}

bool Remote::isIrRemote() const {
    // the core's own integration for IR remotes; Bluetooth remotes are uc_bt.main
    return m_integration == QLatin1String("uc.main");
}

bool Remote::isSimpleCommand(const QString &cmdId) {
    return !cmdId.contains(QLatin1String("remote."));
}

bool Remote::isIrTapOrHold(const QString &cmdId, bool hasLongPress) const {
    return isIrRemote() && isSimpleCommand(cmdId) && !hasLongPress;
}

void Remote::sendCommand(RemoteCommands::Enum cmd, QVariantMap params) {
    Base::sendCommand(QVariant::fromValue(cmd).toString(), params);
}

void Remote::sendCommand(RemoteCommands::Enum cmd) { sendCommand(cmd, QVariantMap()); }

bool Remote::updateAttribute(const QString &attribute, QVariant data) {
    bool ok = false;

    // convert to enum
    RemoteAttributes::Enum attributeEnum = Util::convertStringToEnum<RemoteAttributes::Enum>(attribute);

    switch (attributeEnum) {
        case RemoteAttributes::State: {
            int newState = Util::convertStringToEnum<RemoteStates::Enum>(uc::Util::FirstToUpper(data.toString()));
            if (m_state != newState && newState != -1) {
                m_state = newState;
                ok      = true;
                emit stateChanged(m_id, m_state);

                m_stateAsString = RemoteStates::getTranslatedString(static_cast<RemoteStates::Enum>(m_state));
                emit stateAsStringChanged();

                m_stateInfo = getStateAsString();
                emit stateInfoChanged();
            }
            break;
        }
    }

    return ok;
}

bool Remote::updateOptions(QVariant data) {
    bool        ok      = false;
    QVariantMap options = data.toMap();

    if (options.contains("user_interface")) {
        m_uiConfig = options.value("user_interface").toMap();
        ok         = true;
        emit uiConfigChanged();
    }

    if (options.contains("button_mapping")) {
        m_buttonMapping = options.value("button_mapping").toList();
        ok              = true;
        emit buttonMappingChanged();
    }

    return ok;
}

void Remote::onLanguageChangedTypeSpecific()
{
    QTimer::singleShot(500, this, [=]() {
        m_stateAsString = RemoteStates::getTranslatedString(static_cast<RemoteStates::Enum>(m_state));
        emit stateAsStringChanged();

        m_stateInfo = getStateAsString();
        emit stateInfoChanged();
    });
}

}  // namespace entity
}  // namespace ui
}  // namespace uc
