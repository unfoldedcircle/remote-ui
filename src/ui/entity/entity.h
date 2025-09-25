// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QVariant>
#include <QCoreApplication>
#include <QTimer>

#include "../../util.h"

namespace uc {
namespace ui {
namespace entity {

class Base : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString id READ getId CONSTANT)
    Q_PROPERTY(QString name READ getName NOTIFY nameChanged)
    Q_PROPERTY(QString icon READ getIcon NOTIFY iconChanged)
    Q_PROPERTY(QString area READ getArea NOTIFY areaChanged)
    Q_PROPERTY(int state READ getState NOTIFY stateChanged)
    Q_PROPERTY(QString stateAsString READ getStateAsString NOTIFY stateAsStringChanged)
    Q_PROPERTY(QString stateInfo READ getStateInfo NOTIFY stateInfoChanged)
    Q_PROPERTY(Type type READ getType CONSTANT)
    Q_PROPERTY(bool enabled READ isEnabled NOTIFY enabledChanged)

 public:
    enum Type { Unsupported, Button, Switch, Climate, Cover, Light, Media_player, Remote, Sensor, Activity, Macro };
    Q_ENUM(Type)

    enum CommonAttributes { Name, Icon, Area };
    Q_ENUM(CommonAttributes)

    explicit Base(const QString &id, QVariantMap nameI18n, const QString &language, const QString &icon,
                  const QString &area, Type type, bool enabled, QVariantMap attributes,
                  const QString &integration = QString(), bool selected = false, QObject *parent = nullptr);
    ~Base();

    // Q_PROPERTY METHODS
    QString             getId() { return m_id; }
    QString             getName() { return m_name; }
    QVariantMap         getNameI18n() { return m_name_i18n; }
    QString             getIcon() { return m_icon; }
    QString             getArea() { return m_area; }
    int                 getState() { return m_state; }
    QString             getStateAsString() { return m_stateAsString; }
    virtual QString     getStateInfo() { return m_stateInfo; }
    Type                getType() { return m_type; }
    Q_INVOKABLE QString getTypeAsString() { return Util::convertEnumToString<Type>(m_type).toLower(); }
    Q_INVOKABLE QString getDeviceClass() { return m_deviceClass; }
    bool                isEnabled() { return m_enabled; }

    Q_INVOKABLE bool hasFeature(int feature);
    Q_INVOKABLE bool hasAllFeatures(QVariantList features);
    Q_INVOKABLE bool hasAnyFeature(QVariantList features);

    virtual Q_INVOKABLE void turnOn() {}
    virtual Q_INVOKABLE void turnOff() {}

    bool setFriendlyName(QVariantMap nameI18n, const QString &language);
    bool setIcon(const QString &icon);
    bool setArea(const QString &area);
    bool setState(int state);

    void sendCommand(const QString &cmd, QVariantMap params);
    void sendCommand(const QString &cmd);

    template <class T>
    bool updateFeatures(const QStringList &features) {
        if (features.size() == 0) {
            return false;
        }

        m_features.clear();

        for (QStringList::const_iterator i = features.begin(); i != features.end(); i++) {
            m_features.append(Util::convertStringToEnum<T>(*i));
        }

        return true;
    }

    virtual bool updateAttribute(const QString &attribute, QVariant data) {
        Q_UNUSED(attribute)
        Q_UNUSED(data)
        return false;
    }

    virtual bool updateOptions(QVariant data) {
        Q_UNUSED(data)
        return false;
    }

    QString getIntegration() { return m_integration; }
    QString getSorting() { return m_name.toLower() + m_integration.toLower() + m_id.toLower() + m_area.toLower(); }
    bool    getSelected() { return m_selected; }
    void    setSelected(bool selected) { m_selected = selected; }

    virtual void onLanguageChangedTypeSpecific() {}

 public:
    static Type typeFromString(const QString &key, bool *ok = nullptr) {
        return Util::convertStringToEnum<Type>(key, ok);
    }

 signals:
    void nameChanged();
    void iconChanged();
    void areaChanged();
    void stateChanged(QString entityId, int newState);
    void stateAsStringChanged();
    void stateInfoChanged();
    void enabledChanged();

    void command(QString entityId, QString command, QVariantMap parmas);

    void uiOpened();
    void uiClosed();

 public slots:
    void onLanguageChanged(QString language);
    void onStateChanged(QString entityId, int newState);

 protected:
    QString     m_id;
    QString     m_name;
    QVariantMap m_name_i18n;
    QString     m_icon;
    QString     m_area;
    int         m_state;
    QString     m_stateAsString;
    QString     m_stateInfo;
    Type        m_type;
    QString     m_deviceClass;
    bool        m_enabled;

    QList<int> m_features;

    QString m_integration;
    bool    m_selected;
};

}  // namespace entity
}  // namespace ui
}  // namespace uc
