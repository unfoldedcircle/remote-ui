// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "group.h"

#include "../../logging.h"
#include "../notification.h"

namespace uc {
namespace ui {

GroupItemList::GroupItemList(QObject *parent) : QAbstractListModel(parent), m_count(0) {}

int GroupItemList::count() const {
    //    return m_count;
    return m_data.size();
}

void GroupItemList::append(GroupItem *item) {
    if (contains(item->groupItemId())) {
        return;
    }

    int i = m_data.size();
    beginInsertRows(QModelIndex(), i, i);
    m_data.append(item);

    emit countChanged(count());
    endInsertRows();
}

int GroupItemList::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return m_data.size();
}

bool GroupItemList::removeRows(int row, int count, const QModelIndex &parent) {
    beginRemoveRows(parent, row, row + count - 1);
    for (int i = row + count - 1; i >= row; i--) {
        m_data.at(i)->deleteLater();
        m_data.removeAt(i);
    }
    endRemoveRows();
    return true;
}

QVariant GroupItemList::data(const QModelIndex &index, int role) const {
    if (index.row() < 0 || index.row() >= m_data.count()) {
        return QVariant();
    }
    const GroupItem *item = m_data[index.row()];
    switch (role) {
        case KeyRole:
            return item->groupItemId();
    }
    return QVariant();
}

QHash<int, QByteArray> GroupItemList::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[KeyRole] = "groupItemId";
    return roles;
}

void GroupItemList::clear() {
    for (int i = 0; i < m_data.count(); i++) {
        m_data[i]->deleteLater();
    }

    beginResetModel();
    m_data.clear();
    endResetModel();
    emit countChanged(count());
}

GroupItem *GroupItemList::getGroupItem(int row) {
    return m_data[row];
}

GroupItem *GroupItemList::getGroupItem(const QString &key) {
    return m_data[getModelIndexByKey(key).row()];
}

QModelIndex GroupItemList::getModelIndexByKey(const QString &key) {
    QModelIndex idx;

    for (int i = 0; i < m_data.count(); i++) {
        if (m_data[i]->groupItemId() == key) {
            idx = index(i, 0);
            break;
        }
    }

    return idx;
}

bool GroupItemList::contains(const QString &key) {
    for (int i = 0; i < m_data.count(); i++) {
        if (m_data[i]->groupItemId() == key) {
            return true;
        }
    }

    return false;
}

void GroupItemList::addItem(const QString &key) {
    append(new GroupItem(key, this));
}

void GroupItemList::removeItem(const QString &key) {
    removeItem(getModelIndexByKey(key).row());
}

void GroupItemList::removeItem(int row) {
    removeRows(row, 1, QModelIndex());
}

void GroupItemList::swapData(int from, int to) {
    if (0 <= from && from < m_data.size() && 0 <= to && to < m_data.size() && from != to) {
        if (from == to - 1) {
            to = from++;
        }

        QModelIndex fromIdx = index(from, 0);
        QModelIndex toIdx = index(to, 0);

        beginMoveRows(QModelIndex(), from, from, QModelIndex(), to);
        m_data.move(from, to);
        endMoveRows();

        emit dataChanged(fromIdx, toIdx);
    }
}

void GroupItemList::setCount(int count) {
    if (m_count == count) {
        return;
    }

    m_count = count;
    emit countChanged(m_count);
}

Group::Group(const QString &id, const QString &profileId, const QString &name, const QString &icon, QObject *parent)
    : QObject(parent), m_id(id), m_profileId(profileId), m_name(name), m_icon(icon) {
    qCDebug(lcGroup()) << "Group created:" << m_id << m_name;
    m_items = new GroupItemList(this);
}

Group::~Group() {
    qCDebug(lcGroup()) << "Group destroyed" << m_id << m_name;
}

void Group::init(const QStringList &items) {
    addEntities(items);
}

QStringList Group::getEntities() {
    QStringList list;

    for (int i = 0; i < m_items->count(); i++) {
        list.append(m_items->getGroupItem(i)->groupItemId());
    }

    return list;
}

void Group::clearEntities() {
    m_items->clear();
}

void Group::addEntity(const QString &entityId) {
    qCDebug(lcGroup()) << "Add entity" << entityId;

    if (m_items->contains(entityId)) {
        qCDebug(lcPage()) << "Already has this" << entityId;
        Notification::createNotification(tr("%1 already exists in this group.").arg(entityId));
        return;
    }

    emit requestEntity(entityId);
    m_items->addItem(entityId);
}

void Group::removeEntity(const QString &entityId) {
    if (m_items->contains(entityId)) {
        qCDebug(lcGroup()) << "Remove entity" << entityId;
        m_items->removeItem(entityId);
    }
}

void Group::addEntities(const QStringList &entities) {
    if (entities.size() > 0) {
        for (QStringList::const_iterator i = entities.begin(); i != entities.end(); i++) {
            addEntity(*i);
        }
    }
}

}  // namespace ui
}  // namespace uc
