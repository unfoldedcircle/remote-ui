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
    return m_rows.size();
}

bool Entities::removeRows(int row, int count, const QModelIndex &parent) {
    if (row < 0 || count <= 0 || (row + count) > m_rows.size()) {
        return false;
    }

    beginRemoveRows(parent, row, row + count - 1);
    for (int i = row + count - 1; i >= row; i--) {
        entity::Base *item = m_rows.takeAt(i);
        m_rowById.remove(item->getId());
        item->deleteLater();
    }

    for (int i = row; i < m_rows.size(); ++i) {
        m_rowById[m_rows.at(i)->getId()] = i;
    }
    endRemoveRows();
    return true;
}

QVariant Entities::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size()) {
        return QVariant();
    }

    entity::Base *item = m_rows.at(index.row());
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
    for (entity::Base *item : qAsConst(m_rows)) {
        item->deleteLater();
    }

    beginResetModel();
    m_rows.clear();
    m_rowById.clear();
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
    return m_rowById.contains(key);
}

void Entities::selectAll() {
    bool changed = false;

    for (entity::Base *item : qAsConst(m_rows)) {
        if (!item->getSelected()) {
            item->setSelected(true);
            changed = true;
        }
    }

    if (changed && !m_rows.isEmpty()) {
        emit dataChanged(index(0, 0), index(m_rows.size() - 1, 0), QVector<int>{SelectedRole});
    }

    m_allSelected = true;
    emit allSelectedChanged();
}

void Entities::clearSelected() {
    bool changed = false;

    for (entity::Base *item : qAsConst(m_rows)) {
        if (item->getSelected()) {
            item->setSelected(false);
            changed = true;
        }
    }

    if (changed && !m_rows.isEmpty()) {
        emit dataChanged(index(0, 0), index(m_rows.size() - 1, 0), QVector<int>{SelectedRole});
    }

    m_allSelected = false;
    emit allSelectedChanged();
}

void Entities::setSelected(const QString &entityId, bool value) {
    auto it = m_rowById.constFind(entityId);
    if (it == m_rowById.cend()) {
        return;
    }

    entity::Base *item = m_rows.at(*it);
    if (item->getSelected() == value) {
        return;
    }

    item->setSelected(value);

    const QModelIndex itemIndex = index(*it, 0);
    emit dataChanged(itemIndex, itemIndex, QVector<int>{SelectedRole});
}

QStringList Entities::getSelected() {
    QStringList list;

    for (entity::Base *entity : qAsConst(m_rows)) {
        if (entity->getSelected()) {
            list.append(entity->getId());
        }
    }

    return list;
}

void Entities::add(entity::Base *o) {
    const QString key = o->getId();

    if (m_rowById.contains(key)) {
        return;
    }

    const int row = m_rows.size();

    beginInsertRows(QModelIndex(), row, row);
    m_rows.append(o);
    m_rowById.insert(key, row);
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
    auto it = m_rowById.constFind(key);
    if (it == m_rowById.cend()) {
        return QModelIndex();
    }

    return index(*it, 0);
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
