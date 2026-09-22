// Copyright (c) 2022-2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "select.h"

#include "../../logging.h"
#include "../../util.h"

namespace uc {
namespace ui {
namespace entity {

Select::Select(const QString &id, QVariantMap nameI18n, const QString &language, const QString &icon, const QString &area,
               const QString &deviceClass, bool enabled, QVariantMap attributes, const QString &integrationId, QObject *parent)
    : Base(id, nameI18n, language, icon, area, Type::Select, enabled, attributes, integrationId, false, parent) {
    qCDebug(lcSelect()) << "Select entity constructor" << deviceClass;

            // attributes
    if (attributes.size() > 0) {
        for (QVariantMap::iterator i = attributes.begin(); i != attributes.end(); i++) {
            updateAttribute(uc::Util::FirstToUpper(i.key()), i.value());
        }
    }

            // device class
    int deviceClassEnum = -1;

    if (!deviceClass.isEmpty()) {
        deviceClassEnum = Util::convertStringToEnum<SelectDeviceClass::Enum>(deviceClass);
    }

    if (deviceClassEnum != -1) {
        m_deviceClass = deviceClass;
    } else {
        m_deviceClass = QVariant::fromValue(SelectDeviceClass::Select).toString();
    }
}

Select::~Select() {
    qCDebug(lcSelect()) << "Select entity destructor";
}

void Select::selectOption(const QString &option)
{
    QVariantMap params;
    params.insert("option", option);
    sendCommand(SelectCommands::Select_option, params);
}

void Select::selectFirst()
{
    sendCommand(SelectCommands::Select_first);
}

void Select::selectLast()
{
    sendCommand(SelectCommands::Select_last);
}

void Select::selectNext()
{
    QVariantMap params;
    params.insert("cycle", true);
    sendCommand(SelectCommands::Select_next, params);
}

void Select::selectPrevious()
{
    QVariantMap params;
    // stepping wraps around in both directions: a key press must never be a no-op on the remote
    params.insert("cycle", true);
    sendCommand(SelectCommands::Select_previous, params);
}

void Select::sendCommand(SelectCommands::Enum cmd, QVariantMap params) {
    Base::sendCommand(QVariant::fromValue(cmd).toString(), params);
}

void Select::sendCommand(SelectCommands::Enum cmd) {
    sendCommand(cmd, QVariantMap());
}

bool Select::updateAttribute(const QString &attribute, QVariant data) {
    bool ok = false;

            // convert to enum
    SelectAttributes::Enum attributeEnum = Util::convertStringToEnum<SelectAttributes::Enum>(attribute);

    switch (attributeEnum) {
        case SelectAttributes::State: {
            int newState = Util::convertStringToEnum<SelectStates::Enum>(uc::Util::FirstToUpper(data.toString()));

            if (m_state != newState && newState != -1) {
                m_state = newState;
                ok = true;
                emit stateChanged(m_id, m_state);

                m_stateAsString = SelectStates::getTranslatedString(static_cast<SelectStates::Enum>(m_state));
                emit stateAsStringChanged();

                m_stateInfo = getStateInfoText();
                emit stateInfoChanged();
            }
            break;
        }
        case SelectAttributes::Options: {
            m_options = data.toStringList();
            emit optionsChanged();
            break;
        }
        case SelectAttributes::Current_option: {
            m_currentOption = data.toString();
            emit currentOptionChanged();

            if (m_state == SelectStates::Enum::On) {
                m_stateInfo = getStateInfoText();
                emit stateInfoChanged();
            }

            break;
        }
    }

    return ok;
}

QString Select::getStateInfoText() const {
    if (m_state != SelectStates::Enum::On) {
        return m_stateAsString;
    }

    if (m_currentOption.isEmpty()) {
        return QCoreApplication::translate("No option is selected in the select entity", "None");
    }

    return m_currentOption;
}

void Select::onLanguageChangedTypeSpecific()
{
    QTimer::singleShot(500, [=]() {
        m_stateAsString = SelectStates::getTranslatedString(static_cast<SelectStates::Enum>(m_state));
        emit stateAsStringChanged();

        // an available select shows the selected option, not the state: keep it after a language change
        m_stateInfo = getStateInfoText();
        emit stateInfoChanged();
    });
}

}  // namespace entity
}  // namespace ui
}  // namespace uc
