// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "integrations.h"

#include "../logging.h"
#include "../util.h"

namespace uc {
namespace integration {

Integration::Integration(const QString &key, const QString &driverId, const QString &deviceId, QVariantMap name,
                         const QString &icon, bool enabled, QVariantMap setupData, const QString &language,
                         bool selected, QObject *parent)
    : QObject(parent),
      m_key(key),
      m_driverId(driverId),
      m_deviceId(deviceId),
      m_name_i18n(name),
      m_icon(icon),
      m_setupData(setupData),
      m_enabled(enabled),
      m_language(language),
      m_selected(selected) {
    m_name = Util::getLanguageString(m_name_i18n, m_language);
    m_sorting = m_name + m_driverId;

    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
}

Integration::~Integration() {}

void Integration::setName(const QString &name) {
    m_name = name;
    emit nameChanged();
}

void Integration::setNameI18n(QVariantMap nameI18n) {
    m_name_i18n = nameI18n;
}

void Integration::setIcon(const QString &icon) {
    m_icon = icon;
    emit iconChanged();
}

void Integration::setEnabled(bool enabled) {
    m_enabled = enabled;
    emit enabledChanged();
}

void Integration::setState(QString state) {
    m_state = state;
    emit stateChanged();
}

void Integration::setSetupData(QVariantMap setupData) {
    m_setupData = setupData;
    emit setupDataChanged();
}

void Integration::setSelected(bool selected) {
    m_selected = selected;
}

void Integration::updateLanguage(const QString &language) {
    m_language = language;

    m_name = Util::getLanguageString(m_name_i18n, m_language);
    emit nameChanged();

    m_sorting = m_name + m_driverId;
}

Integrations::Integrations(QObject *parent) : QAbstractListModel(parent), m_count(0) {}

int Integrations::count() const {
    return m_data.size();
}

void Integrations::setCount(int count) {
    if (m_count == count) {
        return;
    }

    m_count = count;
    emit countChanged(m_count);
}

int Integrations::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return m_data.size();
}

bool Integrations::removeRows(int row, int count, const QModelIndex &parent) {
    beginRemoveRows(parent, row, row + count - 1);
    for (int i = row + count - 1; i >= row; i--) {
        m_data.at(i)->deleteLater();
        m_data.removeAt(i);
    }
    endRemoveRows();
    return true;
}

QVariant Integrations::data(const QModelIndex &index, int role) const {
    if (index.row() < 0 || index.row() >= m_data.count()) {
        return QVariant();
    }
    const Integration *item = m_data[index.row()];
    switch (role) {
        case KeyRole:
            return item->getId();
        case DriverIdRole:
            return item->getDriverId();
        case DeviceIdRole:
            return item->getDeviceId();
        case NameRole:
            return item->getName();
        case IconRole:
            return item->getIcon();
        case EnabledRole:
            return item->getEnabled();
        case StateRole:
            return item->getState();
        case SetupDataRole:
            return item->getSetupData();
        case SelectedRole:
            return item->getSelected();
        case SortingRole:
            return item->getSorting();
    }
    return QVariant();
}

QHash<int, QByteArray> Integrations::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[KeyRole] = "integrationId";
    roles[DriverIdRole] = "driverId";
    roles[DeviceIdRole] = "deviceId";
    roles[NameRole] = "integrationName";
    roles[IconRole] = "integrationIcon";
    roles[EnabledRole] = "integrationEnabled";
    roles[StateRole] = "integrationState";
    roles[SetupDataRole] = "integrationSetupData";
    roles[SelectedRole] = "integrationSelected";
    roles[SortingRole] = "sorted";
    return roles;
}

void Integrations::append(Integration *o) {
    //    int i = m_data.size();
    //    emit layoutAboutToBeChanged();

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_data.append(o);

    emit countChanged(count());
    endInsertRows();

    //    emit layoutChanged();
}

void Integrations::clear() {
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

QModelIndex Integrations::getModelIndexByKey(const QString &key) {
    QModelIndex idx;

    for (int i = 0; i < m_data.count(); i++) {
        if (m_data[i]->getId() == key) {
            idx = index(i, 0);
            break;
        }
    }

    return idx;
}

bool Integrations::contains(const QString &key) {
    for (int i = 0; i < m_data.count(); i++) {
        if (m_data[i]->getId().contains(key)) {
            return true;
        }
    }
    return false;
}

Integration *Integrations::get(const QString &key) {
    //    return m_data[getModelIndexByKey(key).row()];
    for (int i = 0; i < m_data.count(); i++) {
        if (m_data[i]->getId().contains(key)) {
            return m_data[i];
        }
    }
    return nullptr;
}

Integration *Integrations::get(int row) {
    return m_data[row];
}

QString Integrations::getIntegrationIdFromDriverId(const QString &driverId) {
    if (!contains(driverId)) {
        return QString();
    }

    for (int i = 0; i < m_data.count(); i++) {
        if (m_data[i]->getDriverId() == driverId) {
            return m_data[i]->getId();
        }
    }

    return QString();
}

QStringList Integrations::getSelected() {
    QStringList list;

    for (int i = 0; i < m_data.count(); i++) {
        if (m_data[i]->getSelected()) {
            list.append(m_data[i]->getId());
        }
    }

    return list;
}

void Integrations::setSelected(const QString &key, bool selected) {
    get(key)->setSelected(selected);
    emit dataChanged(getModelIndexByKey(key), getModelIndexByKey(key));
}

void Integrations::setState(const QString &key, QString state) {
    auto integration = get(key);

    if (integration) {
        integration->setState(state);
        emit dataChanged(getModelIndexByKey(key), getModelIndexByKey(key));

        qCDebug(lcIntegrationController())
            << "Integration state changed" << integration->getState() << integration->getId();
    }
}

void Integrations::setLanguage(const QString &language) {
    for (int i = 0; i < m_data.count(); i++) {
        m_data[i]->updateLanguage(language);
        emit dataChanged(index(i, 0), index(i, 0));
    }
}

void Integrations::removeItem(const QString &key) {
    removeItem(getModelIndexByKey(key).row());
}

void Integrations::removeItem(int row) {
    removeRows(row, 1, QModelIndex());
    emit countChanged(count());
}

}  // namespace integration
}  // namespace uc
