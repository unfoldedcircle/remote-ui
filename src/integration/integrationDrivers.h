// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QQmlEngine>
#include <QVariant>

#include "setupSchema.h"

namespace uc {
namespace integration {

/**
 * @brief Item for the Integrations model
 */
class IntegrationDriver : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString id READ getId CONSTANT)
    Q_PROPERTY(QString name READ getName NOTIFY nameChanged)
    Q_PROPERTY(QString driverUrl READ getDriverUrl NOTIFY driverUrlChanged)
    Q_PROPERTY(QString version READ getVersion NOTIFY versionChanged)
    Q_PROPERTY(QString icon READ getIcon NOTIFY iconChanged)
    Q_PROPERTY(bool enabled READ getEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QString state READ getState NOTIFY stateChanged)
    Q_PROPERTY(QString description READ getDescription NOTIFY descriptionChanged)
    Q_PROPERTY(QString developerName READ getDeveloperName NOTIFY developerNameChanged)
    Q_PROPERTY(QString homepage READ getHomePage NOTIFY homepageChanged)
    Q_PROPERTY(QString releaseDate READ getReleaseDate NOTIFY releaseDateChanged)
    Q_PROPERTY(bool external READ getExternal CONSTANT)
    Q_PROPERTY(bool discovered READ getExternal NOTIFY discoveredChanged)
    Q_PROPERTY(QObject* setupSchema READ getSetupSchema NOTIFY setupSchemaChanged)
    Q_PROPERTY(int instanceCount READ getInstanceCount NOTIFY instanceCountChanged)

 public:
    explicit IntegrationDriver(const QString& key, QVariantMap name, const QString& driverUrl, const QString& version,
                               const QString& icon, bool enabled, const QString& state, const QString& description,
                               const QString& developerName, const QString& homePage, const QString& releaseDate,
                               SetupSchema* setupSchema, bool discovered, int instanceCount, const QString& language,
                               bool selected = false, bool external = false, QObject* parent = nullptr);
    ~IntegrationDriver();

    QString     getId() const { return m_id; }
    void        setName(const QString& name);
    QString     getName() const { return m_name; }
    void        setNameI18n(QVariantMap nameI18n);
    QVariantMap getNameI18n() const { return m_name_i18n; }
    void        setDriverUrl(const QString& driverUrl);
    QString     getDriverUrl() const { return m_driverUrl; }
    void        setVersion(const QString& version);
    QString     getVersion() const { return m_version; }
    void        setIcon(const QString& icon);
    QString     getIcon() const { return m_icon; }
    void        setEnabled(bool enabled);
    bool        getEnabled() const { return m_enabled; }
    void        setState(QString state);
    QString     getState() const { return m_state; }
    void        setDescription(const QString& description);
    QString     getDescription() const { return m_description; }
    void        setDeveloperName(const QString& developerName);
    QString     getDeveloperName() const { return m_developerName; }
    void        setHomePage(const QString& homePage);
    QString     getHomePage() const { return m_homePage; }
    void        setReleaseDate(const QString& releaseDate);
    QString     getReleaseDate() const { return m_releaseDate; }
    bool        getExternal() const { return m_external; }
    void        setDiscovered(bool discovered);
    bool        getDiscovered() const { return m_discovered; }
    void        setSetupScehma(SetupSchema* setupSchema);
    QObject*    getSetupSchema() { return m_setupSchema; }
    void        setInstanceCount(int count);
    int         getInstanceCount() { return m_instanceCount; }

    void    setSelected(bool selected) { m_selected = selected; }
    bool    getSelected() const { return m_selected; }
    QString getSorting() const { return m_sorting; }

    void updateLanguage(const QString& language);

 signals:
    void idChanged();
    void nameChanged();
    void driverUrlChanged();
    void versionChanged();
    void iconChanged();
    void enabledChanged();
    void stateChanged();
    void descriptionChanged();
    void developerNameChanged();
    void homepageChanged();
    void releaseDateChanged();
    void discoveredChanged();
    void setupSchemaChanged();
    void instanceCountChanged();

 private:
    QString      m_id;
    QString      m_name;
    QVariantMap  m_name_i18n;
    QString      m_driverUrl;
    QString      m_version;
    QString      m_icon;
    bool         m_enabled;
    QString      m_state;
    QString      m_description;
    QString      m_developerName;
    QString      m_homePage;
    QString      m_releaseDate;
    SetupSchema* m_setupSchema;
    bool         m_external;
    bool         m_discovered;
    int          m_instanceCount;

    QString m_language;

    /**
     * @brief to enable the proxy model filterin on multiple roles, we use a sort role
     * and store all searchable string in this variable
     */
    QString m_sorting;
    bool    m_selected;
};

/**
 * @brief This model containts integration driverss
 */
class IntegrationDrivers : public QAbstractListModel {
    Q_OBJECT

    Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged)

 public:
    enum SearchRoles {
        KeyRole = Qt::UserRole + 1,
        NameRole,
        DriverUrlRole,
        VersionRole,
        IconRole,
        EnabledRole,
        StateRole,
        DescriptionRole,
        DeveloperNameRole,
        HomePageRole,
        ReleaseDateRole,
        SetupSchemaRole,
        SelectedRole,
        SortingRole,
        ExternalRole,
        DiscoveredRole,
    };

    explicit IntegrationDrivers(QObject* parent = nullptr);
    ~IntegrationDrivers() = default;

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
    void append(IntegrationDriver* o);
    void clear();

    QModelIndex getModelIndexByKey(const QString& key);

    bool contains(const QString& key);

    IntegrationDriver* get(const QString& key);
    IntegrationDriver* get(int row);

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
    int                       m_count;
    QList<IntegrationDriver*> m_data;
};

}  // namespace integration
}  // namespace uc
