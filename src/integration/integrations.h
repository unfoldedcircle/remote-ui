// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QQmlEngine>
#include <QVariant>

namespace uc {
namespace integration {

/**
 * @brief Item for the Integrations model
 */
class Integration : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString id READ getId CONSTANT)
    Q_PROPERTY(QString driverId READ getDriverId CONSTANT)
    Q_PROPERTY(QString deviceId READ getDeviceId CONSTANT)

    Q_PROPERTY(QString name READ getName NOTIFY nameChanged)
    Q_PROPERTY(QString icon READ getIcon NOTIFY iconChanged)
    Q_PROPERTY(bool enabled READ getEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QString state READ getState NOTIFY stateChanged)
    Q_PROPERTY(QVariantMap setupData READ getSetupData NOTIFY setupDataChanged)

 public:
    explicit Integration(const QString& key, const QString& driverId, const QString& deviceId, QVariantMap name,
                         const QString& icon, bool enabled, QVariantMap setupData, const QString& language,
                         bool selected = false, QObject* parent = nullptr);
    ~Integration();

    QString getId() const { return m_key; }
    QString getDriverId() const { return m_driverId; }
    QString getDeviceId() const { return m_deviceId; }

    QString     getName() const { return m_name; }
    /**
     * @brief Set a new language text map.
     * Note: the integration name is not updated, the client has to call updateLanguage afterwards!
     * @param nameI18n language text map
     */
    void        setNameI18n(QVariantMap nameI18n);
    QVariantMap getNameI18n() const { return m_name_i18n; }
    void        setIcon(const QString& icon);
    QString     getIcon() const { return m_icon; }
    void        setEnabled(bool enabled);
    bool        getEnabled() const { return m_enabled; }
    void        setState(QString state);
    QString     getState() const { return m_state; }
    void        setSetupData(QVariantMap setupData);
    QVariantMap getSetupData() const { return m_setupData; }
    void        setSelected(bool selected);
    bool        getSelected() const { return m_selected; }
    /**
     * @brief to enable the proxy model filtering on multiple roles, we use a sort role
     * and return all searchable string
     */
    QString     getSorting() const { return m_name + m_driverId; }

    void updateLanguage(const QString& language);

 signals:
    void nameChanged();
    void iconChanged();
    void enabledChanged();
    void stateChanged();
    void setupDataChanged();

 private:
    QString     m_key;
    QString     m_driverId;
    QString     m_deviceId;
    QString     m_name;
    QVariantMap m_name_i18n;
    QString     m_icon;
    QVariantMap m_setupData;
    bool        m_enabled;
    QString     m_state;

    QString m_language;
    bool    m_selected;
};

/**
 * @brief This model containts integrations
 */
class Integrations : public QAbstractListModel {
    Q_OBJECT

    Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged)

 public:
    enum SearchRoles {
        KeyRole = Qt::UserRole + 1,
        DriverIdRole,
        DeviceIdRole,
        NameRole,
        IconRole,
        EnabledRole,
        StateRole,
        SetupDataRole,
        SelectedRole,
        SortingRole
    };

    explicit Integrations(QObject* parent = nullptr);
    ~Integrations() = default;

 public:
    // Q_PROPERTY methods
    int count() const;

 public slots:
    // Q_PROPERTY methods
    void setCount(int count);

 public:
    // QQAbstractListModel overrides and required emthods
    int                    rowCount(const QModelIndex& parent = QModelIndex()) const override;
    bool                   removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    QVariant               data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

 public:
    // Custom methods
    void append(Integration* o);
    void clear();

    QModelIndex getModelIndexByKey(const QString& key);

    bool contains(const QString& key);

    Integration* get(const QString& key);
    Integration* get(int row);
    QString      getIntegrationIdFromDriverId(const QString& driverId);

    QStringList getSelected();
    void        setSelected(const QString& key, bool selected);

    void setState(const QString& key, QString state);
    void setLanguage(const QString& language);

    void removeItem(const QString& key);
    void removeItem(int row);

    int totalPages = 0;
    int lastPageLoaded = 0;
    int totalItems = 0;
    int limit = 0;

 signals:
    void countChanged(int count);

 private:
    int                 m_count;
    QList<Integration*> m_data;
};

}  // namespace integration
}  // namespace uc
