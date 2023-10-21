// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "profiles.h"

#include "../../logging.h"

namespace uc {
namespace ui {

Profiles::Profiles(QObject *parent) : QAbstractListModel(parent), m_count(0) {}

//============================================================================================================================================//
// Q_PROPERTY methods

int Profiles::count() const {
    return m_data.size();
}

void Profiles::setCount(int count) {
    if (m_count == count) {
        return;
    }

    m_count = count;
    emit countChanged(m_count);
}

//============================================================================================================================================//
// QQAbstractListModel overrides and required emthods

int Profiles::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return m_data.size();
}

bool Profiles::removeRows(int row, int count, const QModelIndex &parent) {
    beginRemoveRows(parent, row, row + count - 1);
    for (int i = row + count - 1; i >= row; i--) {
        Profile *profile = m_data.at(i);
        m_data.removeAt(i);
        profile->deleteLater();
    }
    endRemoveRows();
    return true;
}

QVariant Profiles::data(const QModelIndex &index, int role) const {
    if (index.row() < 0 || index.row() >= m_data.count()) {
        return QVariant();
    }
    Profile *item = m_data[index.row()];
    switch (role) {
        case KeyRole:
            return item->getId();
        case NameRole:
            return item->getName();
        case IconRole:
            return item->getIcon();
        case RestrictedRole:
            return item->restricted();
    }
    return QVariant();
}

bool Profiles::setData(const QModelIndex &index, const QVariant &value, int role) {
    bool ret = false;

    Profile *profile = m_data[index.row()];

    if (!profile) {
        return ret;
    }

    switch (role) {
        case NameRole:
            profile->setName(value.toString());
            ret = true;
            break;
        case IconRole:
            profile->setIcon(value.toString());
            ret = true;
            break;
        case RestrictedRole:
            profile->setRestricted(value.toBool());
            ret = true;
            break;
    }

    if (ret) {
        emit dataChanged(index, index);
    }

    return ret;
}

QHash<int, QByteArray> Profiles::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[KeyRole] = "profileId";
    roles[NameRole] = "profileName";
    roles[IconRole] = "profileIcon";
    roles[RestrictedRole] = "profileRestricted";
    return roles;
}

//============================================================================================================================================//

void Profiles::append(Profile *o) {
    int i = m_data.count();

    beginInsertRows(QModelIndex(), i, i);
    m_data.append(o);

    emit countChanged(count());
    endInsertRows();
}

void Profiles::clear() {
    for (int i = 0; i < m_data.count(); i++) {
        m_data[i]->deleteLater();
    }

    beginResetModel();
    m_data.clear();
    endResetModel();
    emit countChanged(count());
}

QModelIndex Profiles::getModelIndexByKey(const QString &key) {
    QModelIndex idx;

    for (int i = 0; i < m_data.count(); i++) {
        if (m_data[i]->getId() == key) {
            idx = index(i, 0);
            break;
        }
    }

    return idx;
}

Profile *Profiles::getProfile(const QString &key) {
    return m_data[getModelIndexByKey(key).row()];
}

Profile *Profiles::getProfile(int row) {
    return m_data[row];
}

void Profiles::removeItem(const QString &key) {
    removeItem(getModelIndexByKey(key).row());
}

void Profiles::removeItem(int row) {
    removeRows(row, 1, QModelIndex());
    emit countChanged(count());
}

void Profiles::updateProfileName(const QString &key, const QString &name) {
    auto i = getModelIndexByKey(key);
    setData(i, name, NameRole);
}

void Profiles::updateProfileIcon(const QString &key, const QString &icon) {
    auto i = getModelIndexByKey(key);
    setData(i, icon, IconRole);
}

void Profiles::updateProfileRestricted(const QString &key, bool restricted) {
    auto i = getModelIndexByKey(key);
    setData(i, restricted, RestrictedRole);
}

//============================================================================================================================================//
// QML accesible methods

QString Profiles::getProfileId(int row) {
    return m_data[row]->getId();
}

}  // namespace ui
}  // namespace uc
