// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QQmlEngine>

namespace uc {
namespace integration {

class SettingsEnum {
    Q_GADGET

 public:
    enum Type {
        Number,
        Text,
        Textarea,
        Password,
        Checkbox,
        Dropdown,
        Label,
    };
    Q_ENUM(Type);

 private:
    SettingsEnum() {}
};

class SettingsItem : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString id READ getId CONSTANT)
    Q_PROPERTY(QString label READ getLabel CONSTANT)
    Q_PROPERTY(uc::integration::SettingsEnum::Type type READ getType CONSTANT)

 public:
    explicit SettingsItem(const QString &id, QVariantMap labelI18n, SettingsEnum::Type fieldType,
                          const QString &language, QVariantMap data, QObject *parent = nullptr);
    ~SettingsItem();

    QString                             getId() { return m_id; }
    QString                             getLabel() { return m_label; }
    uc::integration::SettingsEnum::Type getType() { return m_type; }

 private:
    QString            m_id;
    QVariantMap        m_label_i18n;
    QString            m_label;
    SettingsEnum::Type m_type;
    QVariantMap        m_data;
};

class SettingsItemNumber : public SettingsItem {
    Q_OBJECT

    Q_PROPERTY(QString value READ getValue CONSTANT)

 public:
    SettingsItemNumber(const QString &id, QVariantMap labelI18n, SettingsEnum::Type fieldType, const QString &language,
                       QVariantMap data, QObject *parent = nullptr);
    ~SettingsItemNumber();

    QString getValue() { return m_value; }

 private:
    QString m_value;
};

class SettingsItemText : public SettingsItem {
    Q_OBJECT

    Q_PROPERTY(QString value READ getValue CONSTANT)

 public:
    SettingsItemText(const QString &id, QVariantMap labelI18n, SettingsEnum::Type fieldType, const QString &language,
                     QVariantMap data, QObject *parent = nullptr);
    ~SettingsItemText();

    QString getValue() { return m_value; }

 private:
    QString m_value;
};

class SettingsItemTextArea : public SettingsItem {
    Q_OBJECT

    Q_PROPERTY(QString value READ getValue CONSTANT)

 public:
    SettingsItemTextArea(const QString &id, QVariantMap labelI18n, SettingsEnum::Type fieldType,
                         const QString &language, QVariantMap data, QObject *parent = nullptr);
    ~SettingsItemTextArea();

    QString getValue() { return m_value; }

 private:
    QString m_value;
};

class SettingsItemPasssword : public SettingsItem {
    Q_OBJECT

    Q_PROPERTY(QString value READ getValue CONSTANT)

 public:
    SettingsItemPasssword(const QString &id, QVariantMap labelI18n, SettingsEnum::Type fieldType,
                          const QString &language, QVariantMap data, QObject *parent = nullptr);
    ~SettingsItemPasssword();

    QString getValue() { return m_value; }

 private:
    QString m_value;
};

class SettingsItemCheckbox : public SettingsItem {
    Q_OBJECT

    Q_PROPERTY(bool value READ getValue CONSTANT)

 public:
    SettingsItemCheckbox(const QString &id, QVariantMap labelI18n, SettingsEnum::Type fieldType,
                         const QString &language, QVariantMap data, QObject *parent = nullptr);
    ~SettingsItemCheckbox();

    bool getValue() { return m_value; }

 private:
    bool m_value;
};

class SettingsItemDropdown : public SettingsItem {
    Q_OBJECT

    Q_PROPERTY(QString value READ getValue CONSTANT)
    Q_PROPERTY(QVariantList model READ getModel CONSTANT)

 public:
    SettingsItemDropdown(const QString &id, QVariantMap labelI18n, SettingsEnum::Type fieldType,
                         const QString &language, QVariantMap data, QObject *parent = nullptr);
    ~SettingsItemDropdown();

    QString      getValue() { return m_value; }
    QVariantList getModel() { return m_model; }

 private:
    QString      m_value;
    QVariantList m_model;
};

class SettingsItemLabel : public SettingsItem {
    Q_OBJECT

    Q_PROPERTY(QString value READ getValue CONSTANT)

 public:
    SettingsItemLabel(const QString &id, QVariantMap labelI18n, SettingsEnum::Type fieldType, const QString &language,
                      QVariantMap data, QObject *parent = nullptr);
    ~SettingsItemLabel();

    QString getValue() { return m_value; }

 private:
    QString m_value;
};

class SetupSchema : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString title READ getTitle CONSTANT)
    Q_PROPERTY(QList<SettingsItem *> settings READ getSettings CONSTANT)

 public:
    explicit SetupSchema(QVariantMap title, QVariantList settings, const QString &language, QObject *parent = nullptr);
    ~SetupSchema();

    QString               getTitle() { return m_title; }
    QList<SettingsItem *> getSettings() { return m_settings; }

 private:
    QVariantMap           m_title_i18n;
    QString               m_title;
    QList<SettingsItem *> m_settings;
};

}  // namespace integration
}  // namespace uc
