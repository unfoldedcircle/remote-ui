// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "integrationDrivers.h"

#include "../logging.h"
#include "../util.h"

namespace uc {
namespace integration {

IntegrationDriver::IntegrationDriver(const QString &key, QVariantMap name, const QString &driverUrl,
                                     const QString &version, const QString &icon, bool enabled, const QString &state,
                                     const QString &description, const QString &developerName, const QString &homePage,
                                     const QString &releaseDate, SetupSchema *setupSchema, bool discovered,
                                     int instanceCount, const QString &language, bool selected, bool external,
                                     QObject *parent)
    : QObject(parent),
      m_id(key),
      m_name_i18n(name),
      m_driverUrl(driverUrl),
      m_version(version),
      m_icon(icon),
      m_enabled(enabled),
      m_state(state),
      m_description(description),
      m_developerName(developerName),
      m_homePage(homePage),
      m_releaseDate(releaseDate),
      m_setupSchema(setupSchema),
      m_external(external),
      m_discovered(discovered),
      m_instanceCount(instanceCount),
      m_language(language),
      m_selected(selected) {
    m_name = Util::getLanguageString(m_name_i18n, m_language);

    m_sorting = m_name + m_description + m_developerName + m_id;

    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
}

IntegrationDriver::~IntegrationDriver() {
    qCDebug(lcIntegrationDriver()) << "IntegrationDriver destructor" << m_id;
}

void IntegrationDriver::setName(const QString &name) {
    m_name = name;
    emit nameChanged();
}

void IntegrationDriver::setNameI18n(QVariantMap nameI18n) {
    m_name_i18n = nameI18n;
}

void IntegrationDriver::setDriverUrl(const QString &driverUrl) {
    m_driverUrl = driverUrl;
    emit driverUrlChanged();
}

void IntegrationDriver::setVersion(const QString &version) {
    m_version = version;
    emit versionChanged();
}

void IntegrationDriver::setIcon(const QString &icon) {
    m_icon = icon;
    emit iconChanged();
}

void IntegrationDriver::setEnabled(bool enabled) {
    m_enabled = enabled;
    emit enabledChanged();
}

void IntegrationDriver::setState(QString state) {
    m_state = state;
    emit stateChanged();
}

void IntegrationDriver::setDescription(const QString &description) {
    m_description = description;
    emit descriptionChanged();
}

void IntegrationDriver::setDeveloperName(const QString &developerName) {
    m_developerName = developerName;
    emit developerNameChanged();
}

void IntegrationDriver::setHomePage(const QString &homePage) {
    m_homePage = homePage;
    emit homepageChanged();
}

void IntegrationDriver::setReleaseDate(const QString &releaseDate) {
    m_releaseDate = releaseDate;
    emit releaseDateChanged();
}

void IntegrationDriver::setDiscovered(bool discovered) {
    m_discovered = discovered;
    emit discoveredChanged();
}

void IntegrationDriver::setSetupScehma(SetupSchema *setupSchema) {
    m_setupSchema = setupSchema;
    emit setupSchemaChanged();
}

void IntegrationDriver::setInstanceCount(int count) {
    m_instanceCount = count;
    emit instanceCountChanged();
}

void IntegrationDriver::updateLanguage(const QString &language) {
    m_language = language;

    m_name = Util::getLanguageString(m_name_i18n, m_language);
    emit nameChanged();

    m_sorting = m_name + m_description + m_developerName + m_id;
}

IntegrationDrivers::IntegrationDrivers(QObject *parent) : QAbstractListModel(parent), m_count(0) {}

int IntegrationDrivers::count() const {
    return m_data.size();
}

void IntegrationDrivers::setCount(int count) {
    if (m_count == count) {
        return;
    }

    m_count = count;
    emit countChanged(m_count);
}

int IntegrationDrivers::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return m_data.size();
}

bool IntegrationDrivers::removeRows(int row, int count, const QModelIndex &parent) {
    beginRemoveRows(parent, row, row + count - 1);
    for (int i = row + count - 1; i >= row; i--) {
        m_data.at(i)->deleteLater();
        m_data.removeAt(i);
    }
    endRemoveRows();
    return true;
}

QVariant IntegrationDrivers::data(const QModelIndex &index, int role) const {
    if (index.row() < 0 || index.row() >= m_data.count()) {
        return QVariant();
    }
    IntegrationDriver *item = m_data[index.row()];
    switch (role) {
        case KeyRole:
            return item->getId();
        case NameRole:
            return item->getName();
        case DriverUrlRole:
            return item->getDriverUrl();
        case VersionRole:
            return item->getVersion();
        case IconRole:
            return item->getIcon();
        case EnabledRole:
            return item->getEnabled();
        case StateRole:
            return item->getState();
        case DescriptionRole:
            return item->getDescription();
        case DeveloperNameRole:
            return item->getDeveloperName();
        case HomePageRole:
            return item->getHomePage();
        case ReleaseDateRole:
            return item->getReleaseDate();
        case SetupSchemaRole:
            return QVariant::fromValue(item->getSetupSchema());
        case SelectedRole:
            return item->getSelected();
        case SortingRole:
            return item->getSorting();
        case ExternalRole:
            return item->getExternal();
        case DiscoveredRole:
            return item->getDiscovered();
    }
    return QVariant();
}

QHash<int, QByteArray> IntegrationDrivers::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[KeyRole] = "driverId";
    roles[NameRole] = "driverName";
    roles[DriverUrlRole] = "driverUrl";
    roles[VersionRole] = "driverVersion";
    roles[IconRole] = "driverIcon";
    roles[EnabledRole] = "driverEnabled";
    roles[StateRole] = "driverState";
    roles[DescriptionRole] = "driverDescription";
    roles[DeveloperNameRole] = "driverDeveloperName";
    roles[HomePageRole] = "driverHomePage";
    roles[ReleaseDateRole] = "driverReleaseDate";
    roles[SetupSchemaRole] = "driverSetupData";
    roles[SelectedRole] = "driverSelected";
    roles[SortingRole] = "driverSorted";
    roles[ExternalRole] = "driverExternal";
    roles[DiscoveredRole] = "driverDiscovered";
    return roles;
}

void IntegrationDrivers::append(IntegrationDriver *o) {
    //    int i = m_data.size();
    if (contains(o->getId())) {
        return;
    }

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_data.append(o);
    endInsertRows();
    emit countChanged(count());
}

void IntegrationDrivers::clear() {
    for (int i = 0; i < m_data.count(); i++) {
        m_data[i]->deleteLater();
    }

    beginResetModel();
    m_data.clear();
    endResetModel();
    emit countChanged(count());

    totalPages = 0;
    lastPageLoaded = 0;
    totalItems = 0;
    limit = 0;
}

QModelIndex IntegrationDrivers::getModelIndexByKey(const QString &key) {
    QModelIndex idx;

    for (int i = 0; i < m_data.count(); i++) {
        if (m_data[i]->getId() == key) {
            idx = index(i, 0);
            break;
        }
    }

    return idx;
}

bool IntegrationDrivers::contains(const QString &key) {
    for (int i = 0; i < m_data.count(); i++) {
        if (m_data[i]->getId().contains(key)) {
            return true;
        }
    }
    return false;
}

IntegrationDriver *IntegrationDrivers::get(const QString &key) {
    for (int i = 0; i < m_data.count(); i++) {
        if (m_data[i]->getId().contains(key)) {
            return m_data[i];
        }
    }
    return nullptr;
}

IntegrationDriver *IntegrationDrivers::get(int row) {
    return m_data[row];
}

QStringList IntegrationDrivers::getSelected() {
    QStringList list;

    for (int i = 0; i < m_data.count(); i++) {
        if (m_data[i]->getSelected()) {
            list.append(m_data[i]->getId());
        }
    }

    return list;
}

void IntegrationDrivers::setSelected(const QString &key, bool selected) {
    get(key)->setSelected(selected);
    emit dataChanged(getModelIndexByKey(key), getModelIndexByKey(key));
}

void IntegrationDrivers::setState(const QString &key, QString state) {
    auto integration = get(key);

    if (integration) {
        integration->setState(state);
        emit dataChanged(getModelIndexByKey(key), getModelIndexByKey(key));

        qCDebug(lcIntegrationDriver()) << "Integration driver state changed" << integration->getId()
                                       << integration->getState();
    }
}

void IntegrationDrivers::setLanguage(const QString &language) {
    for (int i = 0; i < m_data.count(); i++) {
        m_data[i]->updateLanguage(language);
        emit dataChanged(index(i, 0), index(i, 0));
    }
}

void IntegrationDrivers::removeItem(const QString &key) {
    removeItem(getModelIndexByKey(key).row());
}

void IntegrationDrivers::removeItem(int row) {
    removeRows(row, 1, QModelIndex());
    emit countChanged(count());
}

}  // namespace integration
}  // namespace uc
