// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "setupSchema.h"

#include "../logging.h"
#include "../util.h"

namespace uc {
namespace integration {

SettingsItem::SettingsItem(const QString &id, QVariantMap labelI18n, SettingsEnum::Type fieldType,
                           const QString &language, QVariantMap data, QObject *parent)
    : QObject(parent), m_id(id), m_label_i18n(labelI18n), m_type(fieldType), m_data(data) {
    qCDebug(lcIntegrationDriver()) << "Schema item constructor";

    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);

    m_label = Util::getLanguageString(m_label_i18n, language);
}

uc::integration::SettingsItem::~SettingsItem() {
    qCDebug(lcIntegrationDriver()) << "Schema item destructor";
}

SettingsItemNumber::SettingsItemNumber(const QString &id, QVariantMap labelI18n, SettingsEnum::Type fieldType,
                                       const QString &language, QVariantMap data, QObject *parent)
    : SettingsItem(id, labelI18n, fieldType, language, data, parent) {
    m_value = data.value("number").toMap().value("value").toString();
}

SettingsItemNumber::~SettingsItemNumber() {}

SettingsItemText::SettingsItemText(const QString &id, QVariantMap labelI18n, SettingsEnum::Type fieldType,
                                   const QString &language, QVariantMap data, QObject *parent)
    : SettingsItem(id, labelI18n, fieldType, language, data, parent) {
    m_value = data.value("text").toMap().value("value").toString();
}

SettingsItemText::~SettingsItemText() {}

SettingsItemTextArea::SettingsItemTextArea(const QString &id, QVariantMap labelI18n, SettingsEnum::Type fieldType,
                                           const QString &language, QVariantMap data, QObject *parent)
    : SettingsItem(id, labelI18n, fieldType, language, data, parent) {
    m_value = data.value("textarea").toMap().value("value").toString();
}

SettingsItemTextArea::~SettingsItemTextArea() {}

SettingsItemPasssword::SettingsItemPasssword(const QString &id, QVariantMap labelI18n, SettingsEnum::Type fieldType,
                                             const QString &language, QVariantMap data, QObject *parent)
    : SettingsItem(id, labelI18n, fieldType, language, data, parent) {
    m_value = data.value("password").toMap().value("value").toString();
}

SettingsItemPasssword::~SettingsItemPasssword() {}

SettingsItemCheckbox::SettingsItemCheckbox(const QString &id, QVariantMap labelI18n, SettingsEnum::Type fieldType,
                                           const QString &language, QVariantMap data, QObject *parent)
    : SettingsItem(id, labelI18n, fieldType, language, data, parent) {
    m_value = data.value("checkbox").toMap().value("value").toBool();
}

SettingsItemCheckbox::~SettingsItemCheckbox() {}

SettingsItemDropdown::SettingsItemDropdown(const QString &id, QVariantMap labelI18n, SettingsEnum::Type fieldType,
                                           const QString &language, QVariantMap data, QObject *parent)
    : SettingsItem(id, labelI18n, fieldType, language, data, parent) {
    QVariantList list = data.value("dropdown").toMap().value("items").toList();

    for (QVariantList::const_iterator i = list.cbegin(); i != list.cend(); ++i) {
        QVariantMap item;
        QString     label;

        label = Util::getLanguageString(i->toMap().value("label").toMap(), language);

        item.insert("id", i->toMap().value("id").toString());
        item.insert("label", label);

        m_model.append(item);
    }
}

SettingsItemDropdown::~SettingsItemDropdown() {}

SettingsItemLabel::SettingsItemLabel(const QString &id, QVariantMap labelI18n, SettingsEnum::Type fieldType,
                                     const QString &language, QVariantMap data, QObject *parent)
    : SettingsItem(id, labelI18n, fieldType, language, data, parent) {
    m_value = Util::getLanguageString(data.value("label").toMap().value("value").toMap(), language);
}

SettingsItemLabel::~SettingsItemLabel() {}

SetupSchema::SetupSchema(QVariantMap title, QVariantList settings, const QString &language, QObject *parent)
    : QObject(parent), m_title_i18n(title) {
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);

    qCDebug(lcIntegrationDriver()) << "Schema" << settings;

    m_title = Util::getLanguageString(m_title_i18n, language);

    for (QVariantList::const_iterator i = settings.cbegin(); i != settings.cend(); ++i) {
        QString     id = i->toMap().value("id").toString();
        QVariantMap label = i->toMap().value("label").toMap();
        QVariantMap data = i->toMap().value("field").toMap();

        SettingsEnum::Type type;

        for (QVariantMap::const_iterator iter = data.cbegin(); iter != data.cend(); ++iter) {
            type = Util::convertStringToEnum<SettingsEnum::Type>(Util::FirstToUpper(iter.key()));
        }

        qCDebug(lcIntegrationDriver()) << "Schema setting" << m_title << id << type << label;

        SettingsItem *settingsItem = nullptr;

        switch (type) {
            case SettingsEnum::Type::Number:
                settingsItem = new SettingsItemNumber(id, label, type, language, data, this);
                break;
            case SettingsEnum::Type::Text:
                settingsItem = new SettingsItemText(id, label, type, language, data, this);
                break;
            case SettingsEnum::Type::Textarea:
                settingsItem = new SettingsItemTextArea(id, label, type, language, data, this);
                break;
            case SettingsEnum::Type::Password:
                settingsItem = new SettingsItemPasssword(id, label, type, language, data, this);
                break;
            case SettingsEnum::Type::Checkbox:
                settingsItem = new SettingsItemCheckbox(id, label, type, language, data, this);
                break;
            case SettingsEnum::Type::Dropdown:
                settingsItem = new SettingsItemDropdown(id, label, type, language, data, this);
                break;
            case SettingsEnum::Type::Label:
                settingsItem = new SettingsItemLabel(id, label, type, language, data, this);
                break;
        }

        if (settingsItem) {
            m_settings.append(settingsItem);
            qCDebug(lcIntegrationDriver()) << "Settings item added" << settingsItem;
        }
    }

    qRegisterMetaType<SettingsEnum::Type>("Setting Schema Types");
    qmlRegisterUncreatableType<SettingsEnum>("Settings.SchemaTypes", 1, 0, "SettingsSchemaTypes", "Enum is not a type");
}

SetupSchema::~SetupSchema() {
    qCDebug(lcIntegrationDriver()) << "Schema destructor";
}

}  // namespace integration
}  // namespace uc
