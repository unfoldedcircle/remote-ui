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

    m_touchTimer.setSingleShot(true);
    connect(&m_touchTimer, &QTimer::timeout, this, &Remote::onTouchTimer);
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

void Remote::touchPressed(const QString &touchId, const QString &cmdId, const QVariantMap &params, bool sendPress) {
    // one finger at a time: a touch that is still running ends first
    endTouch();
    m_touchEndedHoldId.clear();

    const QString command = touchHoldCommand(cmdId, params);
    if (command.isEmpty()) {
        return;
    }

    m_touchId = touchId;
    m_touchCommand = command;
    m_touchSendPress = sendPress;
    m_touchHeld = false;
    m_touchTimer.start(kTouchHoldDelayMs);
}

bool Remote::touchReleased(const QString &touchId) {
    if (!m_touchId.isEmpty() && touchId == m_touchId) {
        return endTouch();
    }

    // the hold of this touch already ended without its release
    if (!m_touchEndedHoldId.isEmpty() && touchId == m_touchEndedHoldId) {
        m_touchEndedHoldId.clear();
        return true;
    }
    return false;
}

void Remote::touchCanceled(const QString &touchId) {
    if (!m_touchId.isEmpty() && touchId == m_touchId) {
        endTouch();
    }
    if (touchId == m_touchEndedHoldId) {
        m_touchEndedHoldId.clear();
    }
}

void Remote::cancelTouch() {
    const QString touchId = m_touchId;
    if (endTouch()) {
        qCDebug(lcRemote()) << "Touch hold ended without its release:" << touchId;
        m_touchEndedHoldId = touchId;
    }
}

QString Remote::touchHoldCommand(const QString &cmdId, const QVariantMap &params) {
    QString command;

    if (isSimpleCommand(cmdId)) {
        command = cmdId;
    } else if ((cmdId.compare(QLatin1String("remote.send"), Qt::CaseInsensitive) == 0 ||
                cmdId.compare(QLatin1String("remote.send_cmd"), Qt::CaseInsensitive) == 0) &&
               params.size() == 1 && params.contains(QStringLiteral("command"))) {
        // a single command with nothing else configured: held like the simple command it sends
        command = params.value(QStringLiteral("command")).toString();
    } else {
        // any other command, a sequence or a command with its own repeat, delay or hold, only taps
        return QString();
    }

    // power commands never repeat; command names of integration remotes are free text, so they are recognised by
    // their name
    static const QStringList kPowerCommands{QStringLiteral("on"), QStringLiteral("off"), QStringLiteral("toggle")};
    if (command.trimmed().isEmpty() || kPowerCommands.contains(command, Qt::CaseInsensitive) ||
        command.contains(QLatin1String("power"), Qt::CaseInsensitive)) {
        return QString();
    }

    return command;
}

void Remote::onTouchTimer() {
    if (m_touchId.isEmpty()) {
        return;
    }

    // a failsafe: a hold whose release is lost ends after the limit
    if (m_touchHeld && m_touchHeldFor.elapsed() >= m_touchHoldMaxMs) {
        qCWarning(lcRemote()) << "Touch hold reached its limit of" << m_touchHoldMaxMs << "ms:" << m_touchId;
        cancelTouch();
        return;
    }

    // restart first: a handler of the repeat may end the touch and stop the timer
    m_touchTimer.start(kTouchRepeatPeriodMs);

    // copies: a handler may end the touch and clear the members
    const QString touchId = m_touchId;
    const QString command = m_touchCommand;
    const bool    sendPress = m_touchSendPress;

    const bool started = !m_touchHeld;
    if (started) {
        m_touchHeld = true;
        m_touchHeldFor.start();
        qCDebug(lcRemote()) << "Touch hold started:" << touchId << command;
    }

    // the first repeat goes out before the hold start is announced: a release in a handler of it ends a hold that
    // has really started
    mappedButtonRepeated(touchId, command, QVariantMap(), sendPress);

    if (started && m_touchId == touchId) {
        emit touchHoldStarted(touchId);
    }
}

bool Remote::endTouch() {
    if (m_touchId.isEmpty()) {
        return false;
    }

    m_touchTimer.stop();

    const QString touchId = m_touchId;
    const QString command = m_touchCommand;
    const bool    held = m_touchHeld;

    m_touchId.clear();
    m_touchCommand.clear();
    m_touchHeld = false;

    if (held) {
        // like the release of a held mapped button without a long press
        mappedButtonReleased(touchId, command, QVariantMap(), false);
    }
    return held;
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
