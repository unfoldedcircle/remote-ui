// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "macro.h"

#include "../../logging.h"
#include "../../util.h"

namespace uc {
namespace ui {
namespace entity {

Macro::Macro(const QString &id, const QString &name, QVariantMap nameI18n, const QString &icon, const QString &area,
             const QString &deviceClass, const QStringList &features, bool enabled, QVariantMap attributes,
             const QString &integrationId, QObject *parent)
    : Base(id, name, nameI18n, icon, area, Type::Macro, enabled, attributes, integrationId, false, parent),
      m_currentStep(this) {
    qCDebug(lcMacro()) << "Macro entity constructor";

    updateFeatures<MacroFeatures::Enum>(features);

    // attributes
    if (attributes.size() > 0) {
        for (QVariantMap::iterator i = attributes.begin(); i != attributes.end(); i++) {
            updateAttribute(uc::Util::FirstToUpper(i.key()), i.value());
        }
    }

    // device class
    int deviceClassEnum = -1;

    if (!deviceClass.isEmpty()) {
        deviceClassEnum = Util::convertStringToEnum<MacroDeviceClass::Enum>(deviceClass);
    }

    if (deviceClassEnum != -1) {
        m_deviceClass = deviceClass;
    } else {
        m_deviceClass = QVariant::fromValue(MacroDeviceClass::Macro).toString();
    }
}

Macro::~Macro() {
    qCDebug(lcMacro()) << "Macro entity destructor";
}

void Macro::turnOn() {
    run();
}

void Macro::run() {
    sendCommand(MacroCommands::Run);
}

void Macro::stop() {
    sendCommand(MacroCommands::Stop);
}

void Macro::clearCurrentStep()
{
    m_totalSteps = 0;
    emit totalStepsChanged();
}

void Macro::sendCommand(MacroCommands::Enum cmd, QVariantMap params) {
    Base::sendCommand(QVariant::fromValue(cmd).toString(), params);
}

void Macro::sendCommand(MacroCommands::Enum cmd) {
    sendCommand(cmd, QVariantMap());
}

bool Macro::updateAttribute(const QString &attribute, QVariant data) {
    bool ok = false;

    // convert to enum
    MacroAttributes::Enum attributeEnum = Util::convertStringToEnum<MacroAttributes::Enum>(attribute);

    switch (attributeEnum) {
        case MacroAttributes::State: {
            int newState = Util::convertStringToEnum<MacroStates::Enum>(uc::Util::FirstToUpper(data.toString()));

            if (newState != -1) {
                m_state = newState;
                ok = true;

                m_stateAsString = MacroStates::getTranslatedString(static_cast<MacroStates::Enum>(m_state));
                emit stateAsStringChanged();
                emit stateChanged(m_id, m_state);
            }
            break;
        }
        case MacroAttributes::Total_steps: {
            m_totalSteps = data.toInt();
            emit totalStepsChanged();
            ok = true;
            break;
        }
        case MacroAttributes::Step: {
            QVariantMap newStep = data.toMap();

            m_currentStep.setType(uc::Util::convertStringToEnum<SequenceStep::Type>(
                uc::Util::FirstToUpper(newStep.value("type").toString())));
            m_currentStep.setCurrentIndex(newStep.value("index").toInt());
            m_currentStep.setDelay(newStep.value("delay").toInt());
            m_currentStep.setEntityId(newStep.value("command").toMap().value("entity_id").toString());
            m_currentStep.setCommandId(newStep.value("command").toMap().value("cmd_id").toString());
            m_currentStep.setError(newStep.value("error").toString());
            emit currentStepChanged();
            ok = true;
            break;
        }
    }

    return ok;
}

void Macro::onLanguageChangedTypeSpecific()
{
    QTimer::singleShot(500, [=]() {
        m_stateAsString = MacroStates::getTranslatedString(static_cast<MacroStates::Enum>(m_state));
        emit stateAsStringChanged();
        emit stateChanged(m_id, m_state);
    });
}

}  // namespace entity
}  // namespace ui
}  // namespace uc
