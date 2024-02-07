// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "entity.h"

#include "../../logging.h"

namespace uc {
namespace ui {
namespace entity {

Base::Base(const QString &id, const QString &name, QVariantMap nameI18n, const QString &icon, const QString &area,
           Type type, bool enabled, QVariantMap attributes, const QString &integration, bool selected, QObject *parent)
    : QObject(parent),
      m_id(id),
      m_name(name),
      m_name_i18n(nameI18n),
      m_icon(icon),
      m_area(area),
      m_state(-1),
      m_type(type),
      m_enabled(enabled),
      m_integration(integration),
      m_selected(selected) {
    qCDebug(lcEntity()) << "Base constructor" << m_id;

    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);

    if (attributes.size() > 0) {
        for (QVariantMap::iterator i = attributes.begin(); i != attributes.end(); i++) {
            updateCommonAttribute(uc::Util::FirstToUpper(i.key()), i.value());
        }
    }

    if (m_name.isEmpty()) {
        m_name = Util::getLanguageString(m_name_i18n, "en");
    }

    onStateChanged(m_id, m_state);

    // default icons
    if (m_icon.isEmpty()) {
        m_icon = "uc:";

        switch (m_type) {
            case Button:
                m_icon.append("power-on");
                break;
            case Climate:
                m_icon.append("climate");
                break;
            case Cover:
                m_icon.append("blind");
                break;
            case Light:
                m_icon.append("light");
                break;
            case Media_player:
                m_icon.append("music");
                break;
            case Sensor:
                m_icon.append("sensor");
                break;
            case Switch:
                m_icon.append("power-on");
                break;
            case Remote:
                m_icon.append("remote");
                break;
            case Activity:
                m_icon.append("activity");
                break;
            case Macro:
                m_icon.append("activity");
                break;
            default:
                m_icon.append("warning");
                break;
        }
    }

    m_sorting = m_name.toLower() + m_integration.toLower() + m_id.toLower() + m_area.toLower();

    QObject::connect(this, &Base::stateChanged, this, &Base::onStateChanged);
}

Base::~Base() {
    qCDebug(lcEntity()) << "Base destructor" << m_id;
}

bool Base::hasFeature(int feature) {
    return m_features.contains(feature);
}

bool Base::hasAllFeatures(QVariantList features) {
    if (features.isEmpty()) {
        return false;
    }

    for (QVariantList::iterator i = features.begin(); i != features.end(); i++) {
        if (!m_features.contains(i->toInt())) {
            return false;
        }
    }

    return true;
}

bool Base::hasAnyFeature(QVariantList features) {
    if (features.isEmpty()) {
        return false;
    }

    for (QVariantList::iterator i = features.begin(); i != features.end(); i++) {
        if (m_features.contains(i->toInt())) {
            return true;
        }
    }

    return false;
}

void Base::sendCommand(const QString &cmd, QVariantMap params) {
    const QString finalCommand = Util::convertEnumToString(m_type).append(".").append(cmd);
    emit          command(m_id, finalCommand.toLower(), params);
}

void Base::sendCommand(const QString &cmd) {
    sendCommand(cmd, QVariantMap());
}

bool Base::updateCommonAttribute(const QString &attribute, QVariant data) {
    bool ok = false;

    CommonAttributes attributeEnum = Util::convertStringToEnum<CommonAttributes>(attribute);

    switch (attributeEnum) {
        case Name: {
            ok = setFriendlyName(data.toString());
            break;
        }
        case Icon: {
            ok = setIcon(data.toString());
            break;
        }
        case Area: {
            ok = setArea(data.toString());
            break;
        }
    }

    return ok;
}

void Base::onLanguageChanged(QString language) {
    m_name = Util::getLanguageString(m_name_i18n, language);
    emit nameChanged();

    m_sorting = m_name.toLower() + m_integration.toLower() + m_id.toLower() + m_area.toLower();

    onLanguageChangedTypeSpecific();
}

void Base::onStateChanged(QString entityId, int newState) {
    Q_UNUSED(entityId)

    if (newState == 0) {
        m_enabled = false;
        emit enabledChanged();
    } else {
        m_enabled = true;
        emit enabledChanged();
    }
}

bool Base::setFriendlyName(const QString &friendlyName) {
    m_name = friendlyName;
    emit nameChanged();
    return true;
}

bool Base::setIcon(const QString &icon) {
    m_icon = icon;
    emit iconChanged();
    return true;
}

bool Base::setArea(const QString &area) {
    m_area = area;
    emit areaChanged();
    return true;
}

bool Base::setState(int state) {
    m_state = state;
    emit stateChanged(m_id, m_state);
    return true;
}

}  // namespace entity
}  // namespace ui
}  // namespace uc
