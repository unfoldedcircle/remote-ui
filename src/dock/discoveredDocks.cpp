// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "discoveredDocks.h"

#include "../logging.h"

namespace uc {
namespace dock {

DiscoveredDock::DiscoveredDock(const QString &id, bool configured, const QString &friendlyName, const QString &address,
                               const QString &model, const QString& revision, const QString& serial, const QString &version, const QString &discoveryType,
                               int bluetoothSignal, int bluetoothLastSeenSeconds, QObject *parent)
    : QObject(parent),
      m_id(id),
      m_configured(configured),
      m_friendlyName(friendlyName),
      m_address(address),
      m_model(model),
      m_revision(revision),
      m_serial(serial),
      m_version(version),
      m_discoveryType(discoveryType),
      m_bluetoothSignal(bluetoothSignal),
      m_bluetoothLastSeenSeconds(bluetoothLastSeenSeconds) {}

DiscoveredDock::~DiscoveredDock() {
    qCDebug(lcDockController()) << "Discovered dock destructor" << m_id;
}

DiscoveredDocks::DiscoveredDocks(QObject *parent) : QAbstractListModel(parent), m_count(0) {}

int DiscoveredDocks::count() const {
    return m_data.size();
}

void DiscoveredDocks::setCount(int count) {
    if (m_count == count) {
        return;
    }

    m_count = count;
    emit countChanged(m_count);
}

int DiscoveredDocks::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return m_data.size();
}

bool DiscoveredDocks::removeRows(int row, int count, const QModelIndex &parent) {
    beginRemoveRows(parent, row, row + count - 1);
    for (int i = row + count - 1; i >= row; i--) {
        m_data.at(i)->deleteLater();
        m_data.removeAt(i);
    }
    endRemoveRows();
    return true;
}

QVariant DiscoveredDocks::data(const QModelIndex &index, int role) const {
    if (index.row() < 0 || index.row() >= m_data.count()) {
        return QVariant();
    }
    const DiscoveredDock *item = m_data[index.row()];
    switch (role) {
        case KeyRole:
            return item->itemId();
        case ConfiguredRole:
            return item->itemConfigured();
        case FirendlyNameRole:
            return item->itemFriendlyName();
        case AddressRole:
            return item->itemAddress();
        case ModelRole:
            return item->itemModel();
        case RevisionRole:
            return item->itemRevision();
        case SerialRole:
            return item->itemSerial();
        case VersionRole:
            return item->itemVersion();
        case DiscoveryTypeRole:
            return item->itemDiscoveryType();
        case BluetoothSignalRole:
            return item->itemBluetoothSignal();
        case BluetoothLastSeenSecondsRole:
            return item->itemBluetoothLastSeenSeconds();
    }
    return QVariant();
}

QHash<int, QByteArray> DiscoveredDocks::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[KeyRole] = "itemId";
    roles[ConfiguredRole] = "itemConfigured";
    roles[FirendlyNameRole] = "itemFriendlyName";
    roles[AddressRole] = "itemAddress";
    roles[ModelRole] = "itemModel";
    roles[RevisionRole] = "itemRevision";
    roles[SerialRole] = "itemSerial";
    roles[VersionRole] = "itemVersion";
    roles[DiscoveryTypeRole] = "itemDiscoveryType";
    roles[BluetoothSignalRole] = "itemBluetoothSignal";
    roles[BluetoothLastSeenSecondsRole] = "itemBluetoothLastSeenSeconds";
    return roles;
}

void DiscoveredDocks::append(DiscoveredDock *o) {
    //    int i = m_data.size();
    emit layoutAboutToBeChanged();

    beginInsertRows(QModelIndex(), 0, 0);
    m_data.append(o);

    emit countChanged(count());
    endInsertRows();

    emit layoutChanged();
}

void DiscoveredDocks::clear() {
    for (int i = 0; i < m_data.count(); i++) {
        m_data[i]->deleteLater();
    }

    beginResetModel();
    m_data.clear();
    endResetModel();
    emit countChanged(count());
}

QModelIndex DiscoveredDocks::getModelIndexByKey(const QString &key) {
    QModelIndex idx;

    for (int i = 0; i < m_data.count(); i++) {
        if (m_data[i]->itemId() == key) {
            idx = index(i, 0);
            break;
        }
    }

    return idx;
}

bool DiscoveredDocks::contains(const QString &key) {
    for (int i = 0; i < m_data.count(); i++) {
        if (m_data[i]->itemId().contains(key)) {
            return true;
        }
    }
    return false;
}

DiscoveredDock *DiscoveredDocks::get(const QString &key) {
    //    return m_data[getModelIndexByKey(key).row()];
    for (int i = 0; i < m_data.count(); i++) {
        if (m_data[i]->itemId().contains(key)) {
            return m_data[i];
        }
    }
    return nullptr;
}

DiscoveredDock *DiscoveredDocks::get(int row) {
    return m_data[row];
}

void DiscoveredDocks::removeItem(const QString &key) {
    removeItem(getModelIndexByKey(key).row());
}

void DiscoveredDocks::removeItem(int row) {
    removeRows(row, 1, QModelIndex());
    emit countChanged(count());
}

}  // namespace dock
}  // namespace uc
