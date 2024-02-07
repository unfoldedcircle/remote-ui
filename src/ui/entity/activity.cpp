// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "activity.h"

#include "../../logging.h"
#include "../../util.h"

namespace uc {
namespace ui {
namespace entity {

Activity::Activity(const QString &id, const QString &name, QVariantMap nameI18n, const QString &icon,
                   const QString &area, const QString &deviceClass, const QStringList &features, bool enabled,
                   QVariantMap attributes, QVariantMap options, const QString &integrationId, QObject *parent)
    : Base(id, name, nameI18n, icon, area, Type::Activity, enabled, attributes, integrationId, false, parent),
      m_currentStep(this) {
    qCDebug(lcActivity()) << "Activity entity constructor";

    updateFeatures<ActivityFeatures::Enum>(features);

    // attributes
    if (attributes.size() > 0) {
        for (QVariantMap::iterator i = attributes.begin(); i != attributes.end(); i++) {
            updateAttribute(uc::Util::FirstToUpper(i.key()), i.value());
        }
    }

    // device class
    int deviceClassEnum = -1;

    if (!deviceClass.isEmpty()) {
        deviceClassEnum = Util::convertStringToEnum<ActivityDeviceClass::Enum>(deviceClass);
    }

    if (deviceClassEnum != -1) {
        m_deviceClass = deviceClass;
    } else {
        m_deviceClass = QVariant::fromValue(ActivityDeviceClass::Activity).toString();
    }

    // options
    if (options.contains("user_interface")) {
        m_uiConfig = options.value("user_interface").toMap();
    }

    if (options.contains("button_mapping")) {
        m_buttonMapping = options.value("button_mapping").toList();
    }

    if (options.contains("included_entities")) {
        m_includedEntities = options.value("included_entities").toList();
    }
}

Activity::~Activity() { qCDebug(lcActivity()) << "Activity entity destructor"; }

QStringList Activity::getIncludedEntities() {
    QStringList list;

    for (QVariantList::iterator i = m_includedEntities.begin(); i != m_includedEntities.end(); ++i) {
        QVariantMap entity = i->toMap();

        Base::Type type = entity::Base::typeFromString(Util::FirstToUpper(entity.value("entity_type").toString()));

        if (type > 0 && type != Base::Type::Macro) {
            list.append(entity.value("entity_id").toString());
        }
    }

    return list;
}

void Activity::turnOn() {
    if (m_state == ActivityStates::On) {
        return;
    }

    sendCommand(ActivityCommands::On);
    emit startedRunning(m_id);
}

void Activity::turnOff() {
    if (m_state == ActivityStates::Off) {
        return;
    }

    sendCommand(ActivityCommands::Off);
    emit startedRunning(m_id);
}

void Activity::playPause() { sendButtonMappingCommand("PLAY"); }

void Activity::volumeUp() { sendButtonMappingCommand("VOLUME_UP"); }

void Activity::volumeDown() { sendButtonMappingCommand("VOLUME_DOWN"); }

void Activity::muteToggle() { sendButtonMappingCommand("MUTE"); }

void Activity::previous() { sendButtonMappingCommand("PREV"); }

void Activity::next() { sendButtonMappingCommand("NEXT"); }

void Activity::clearCurrentStep() {
    m_totalSteps = 0;
    emit totalStepsChanged();
}

void Activity::sendCommand(ActivityCommands::Enum cmd, QVariantMap params) {
    Base::sendCommand(QVariant::fromValue(cmd).toString(), params);
}

void Activity::sendCommand(ActivityCommands::Enum cmd) { sendCommand(cmd, QVariantMap()); }

bool Activity::updateAttribute(const QString &attribute, QVariant data) {
    bool ok = false;

    // convert to enum
    ActivityAttributes::Enum attributeEnum = Util::convertStringToEnum<ActivityAttributes::Enum>(attribute);

    switch (attributeEnum) {
        case ActivityAttributes::State: {
            int newState = Util::convertStringToEnum<ActivityStates::Enum>(uc::Util::FirstToUpper(data.toString()));
            if (newState != -1) {
                m_state = newState;
                ok      = true;

                m_stateAsString = ActivityStates::getTranslatedString(static_cast<ActivityStates::Enum>(m_state));
                m_stateInfo = getStateAsString();

                emit stateAsStringChanged();
                emit stateInfoChanged();
                emit stateChanged(m_id, m_state);

                switch (m_state) {
                    case ActivityStates::On:
                        emit addToActivities(m_id);
                        break;
                    case ActivityStates::Off:
                    case ActivityStates::Error:
                        emit removeFromActivities(m_id);
                        break;
                }
            }
            break;
        }
        case ActivityAttributes::Total_steps: {
            m_totalSteps = data.toInt();
            emit totalStepsChanged();
            ok = true;
            break;
        }
        case ActivityAttributes::Step: {
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

bool Activity::updateOptions(QVariant data) {
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

    if (options.contains("included_entities")) {
        m_includedEntities = options.value("included_entities").toList();
        ok                 = true;
        emit includedEntitiesChanged();
    }

    return ok;
}

void Activity::onLanguageChangedTypeSpecific()
{
    QTimer::singleShot(500, [=]() {
        m_stateAsString = ActivityStates::getTranslatedString(static_cast<ActivityStates::Enum>(m_state));
        m_stateInfo = getStateAsString();

        emit stateAsStringChanged();
        emit stateInfoChanged();
        emit stateChanged(m_id, m_state);
    });
}

void Activity::sendButtonMappingCommand(const QString &buttonName, bool shortPress) {
    QString command, entityId;
    QString press = shortPress ? "short_press" : "long_press";

    for (const auto &mapping : qAsConst(m_buttonMapping)) {
        QVariantMap map = mapping.toMap();

        if (map.value("button").toString() == buttonName) {
            command  = map.value(press).toMap().value("cmd_id").toString();
            entityId = map.value(press).toMap().value("entity_id").toString();

            if (entityId.isEmpty() || command.isEmpty()) {
                return;
            }

            emit sendCommandToEntity(entityId, command);
            break;
        }
    }
}

}  // namespace entity
}  // namespace ui
}  // namespace uc
