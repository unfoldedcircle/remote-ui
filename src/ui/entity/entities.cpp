// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "entities.h"

#include "../../logging.h"

namespace uc {
namespace ui {

Entities::Entities(core::Api *core, QObject *parent) : QAbstractListModel(parent), m_core(core) {}

int Entities::count() const {
    return m_count;
}

int Entities::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return m_data.size();
}

bool Entities::removeRows(int row, int count, const QModelIndex &parent) {
    beginRemoveRows(parent, row, row + count - 1);
    for (int i = row + count - 1; i >= row; i--) {
        QHash<QString, entity::Base *>::const_iterator iter = m_data.constBegin() + i;
        iter.value()->deleteLater();
        m_data.remove(iter.value()->getId());
    }
    endRemoveRows();
    return true;
}

QVariant Entities::data(const QModelIndex &index, int role) const {
    if (index.row() < 0 || index.row() >= m_data.size()) {
        return QVariant();
    }

    QHash<QString, entity::Base *>::const_iterator iter = m_data.constBegin() + index.row();

    entity::Base *item = iter.value();
    switch (role) {
        case KeyRole:
            return item->getId();
        case NameRole:
            return item->getName();
        case IconRole:
            return item->getIcon();
        case IntegrationRole:
            return item->getIntegration();
        case TypeRole:
            return item->getType();
        case SelectedRole:
            return item->getSelected();
    }
    return QVariant();
}

QHash<int, QByteArray> Entities::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[KeyRole] = "itemKey";
    roles[NameRole] = "itemName";
    roles[IconRole] = "itemIcon";
    roles[IntegrationRole] = "itemIntegration";
    roles[TypeRole] = "type";
    roles[SelectedRole] = "itemSelected";
    return roles;
}

void Entities::clear() {
    for (QHash<QString, entity::Base *>::const_iterator i = m_data.cbegin(); i != m_data.cend(); ++i) {
        i.value()->deleteLater();
    }

    beginResetModel();
    m_data.clear();
    endResetModel();
    emit countChanged(count());

    m_totalPages = 0;
    m_lastPageLoaded = 0;
    m_totalItems = 0;
    m_limit = 0;

    m_filtered = false;
    emit filteredChanged();
}

bool Entities::contains(const QString &key) {
    return m_data.contains(key);
}

void Entities::selectAll() {
    for (QHash<QString, entity::Base *>::const_iterator i = m_data.cbegin(); i != m_data.cend(); ++i) {
        i.value()->setSelected(true);
        emit dataChanged(getModelIndexByKey(i.key()), getModelIndexByKey(i.key()));
    }
    m_allSelected = true;
    emit allSelectedChanged();
}

void Entities::clearSelected() {
    for (QHash<QString, entity::Base *>::const_iterator i = m_data.cbegin(); i != m_data.cend(); ++i) {
        i.value()->setSelected(false);
        emit dataChanged(getModelIndexByKey(i.key()), getModelIndexByKey(i.key()));
    }
    m_allSelected = false;
    emit allSelectedChanged();
}

void Entities::setSelected(const QString &entityId, bool value) {
    m_data.value(entityId)->setSelected(value);
    emit dataChanged(getModelIndexByKey(entityId), getModelIndexByKey(entityId));
}

QStringList Entities::getSelected() {
    QStringList list;

    for (entity::Base *entity : qAsConst(m_data)) {
        if (entity->getSelected()) {
            list.append(entity->getId());
        }
    }

    return list;
}

void Entities::add(entity::Base *o) {
    if (m_data.contains(o->getId())) {
        return;
    }

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_data.insert(o->getId(), o);
    endInsertRows();
}

void Entities::remove(const QString &key) {
    remove(getModelIndexByKey(key).row());
}

void Entities::remove(int row) {
    removeRows(row, 1, QModelIndex());
    emit countChanged(count());
}

QModelIndex Entities::getModelIndexByKey(const QString &key) {
    QModelIndex idx;

    QHash<QString, entity::Base *>::const_iterator iter = m_data.constFind(key);

    int d = 0;

    while (iter != m_data.constBegin()) {
        ++d;
        --iter;
    }

    idx = index(d, 0);

    return idx;
}

void Entities::setCount(int count) {
    if (m_count == count) {
        return;
    }

    m_count = count;
    emit countChanged(m_count);
}

bool Entities::canLoadMore() {
    return m_totalPages != m_lastPageLoaded;
}

}  // namespace ui
}  // namespace uc
