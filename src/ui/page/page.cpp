// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "page.h"

#include "../../logging.h"
#include "../notification.h"

namespace uc {
namespace ui {

PageItemList::PageItemList(QObject *parent) : QAbstractListModel(parent), m_count(0) {}

int PageItemList::count() const {
    //    return m_count;
    return m_data.size();
}

void PageItemList::append(PageItem *item) {
    if (contains(item->pageItemId())) {
        qCDebug(lcPage()) << "Already exists" << item->pageItemId();
        return;
    }

    //    int i = m_data.size();
    //    beginInsertRows(QModelIndex(), i, i);
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_data.append(item);

    //    emit countChanged(count());
    endInsertRows();
}

int PageItemList::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return m_data.size();
}

bool PageItemList::removeRows(int row, int count, const QModelIndex &parent) {
    beginRemoveRows(parent, row, row + count - 1);
    for (int i = row + count - 1; i >= row; i--) {
        m_data.at(i)->deleteLater();
        m_data.removeAt(i);
    }
    endRemoveRows();
    return true;
}

QVariant PageItemList::data(const QModelIndex &index, int role) const {
    if (index.row() < 0 || index.row() >= m_data.count()) {
        return QVariant();
    }
    const PageItem *item = m_data[index.row()];
    switch (role) {
        case KeyRole:
            return item->pageItemId();
        case TypeRole:
            return item->pageItemType();
    }
    return QVariant();
}

QHash<int, QByteArray> PageItemList::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[KeyRole] = "pageItemId";
    roles[TypeRole] = "pageItemType";
    return roles;
}

void PageItemList::clear() {
    for (int i = 0; i < m_data.count(); i++) {
        delete m_data[i];
    }

    beginResetModel();
    m_data.clear();
    endResetModel();
    emit countChanged(count());
}

PageItem *PageItemList::getPageItem(int row) {
    return m_data[row];
}

PageItem *PageItemList::getPageItem(const QString &key) {
    return m_data[getModelIndexByKey(key).row()];
}

QModelIndex PageItemList::getModelIndexByKey(const QString &key) {
    QModelIndex idx;

    for (int i = 0; i < m_data.count(); i++) {
        if (m_data[i]->pageItemId() == key) {
            idx = index(i, 0);
            break;
        }
    }

    return idx;
}

bool PageItemList::contains(const QString &key) {
    for (int i = 0; i < m_data.count(); i++) {
        if (m_data[i]->pageItemId().contains(key)) {
            return true;
        }
    }

    return false;
}

void PageItemList::addItem(const QString &key, PageItem::Type type) {
    append(new PageItem(key, type, this));
}

void PageItemList::removeItem(const QString &key) {
    removeItem(getModelIndexByKey(key).row());
}

void PageItemList::removeItem(int row) {
    removeRows(row, 1, QModelIndex());
}

void PageItemList::swapData(int from, int to) {
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

void PageItemList::setCount(int count) {
    if (m_count == count) {
        return;
    }

    m_count = count;
    emit countChanged(m_count);
}

Page::Page(const QString &key, const QString &name, const QString &image, QObject *parent)
    : QObject(parent), m_id(key), m_name(name), m_image(image) {
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);

    m_items = new PageItemList(this);
    m_activities = new PageItemList(this);
}

Page::~Page() {
    m_items->deleteLater();
    m_activities->deleteLater();
    qCDebug(lcPage()) << "Page destroyed" << m_id << m_name;
}

void Page::init(const QList<core::PageItem> &items) {
    // iterate through items and add to page
    if (items.size() > 0) {
        for (QList<core::PageItem>::const_iterator i = items.begin(); i != items.end(); i++) {
            PageItem::Type type = PageItem::typeFromString(i->type);

            switch (type) {
                case PageItem::Entity:
                    addEntity(i->id);
                    break;
                case PageItem::Group:
                    addGroup(i->id);
                    break;
            }
        }
    }
}

void Page::addEntity(const QString &entityId) {
    qCDebug(lcPage()) << "Add entity" << entityId;

    if (m_items->contains(entityId)) {
        qCDebug(lcPage()) << "Already has this" << entityId;
        Notification::createNotification(tr("%1 already exists on the page.").arg(entityId));
        return;
    }

    emit requestEntity(entityId);
    m_items->addItem(entityId, PageItem::Entity);
}

void Page::removeEntity(const QString &entityId) {
    if (m_items->contains(entityId)) {
        qCDebug(lcPage()) << "Remove entity" << entityId;
        m_items->removeItem(entityId);
    }
}

void Page::addEntities(const QStringList &entities) {
    if (entities.size() > 0) {
        for (QStringList::const_iterator i = entities.begin(); i != entities.end(); i++) {
            addEntity(*i);
        }
    }
}

void Page::addGroup(const QString &groupId) {
    qCDebug(lcPage()) << "Add group" << groupId;
    m_items->addItem(groupId, PageItem::Group);
}

void Page::removeGroup(const QString &groupId) {
    if (m_items->contains(groupId)) {
        qCDebug(lcPage()) << "Remove group" << groupId;
        m_items->removeItem(groupId);
    }
}

void Page::addGroups(const QStringList &groups) {
    if (groups.size() > 0) {
        for (QStringList::const_iterator i = groups.begin(); i != groups.end(); i++) {
            addGroup(*i);
        }
    }
}

void Page::removeEntities() {
    m_items->clear();
}

void Page::addActivity(QString entityId) {
    if (!m_activities->contains(entityId)) {
        m_activities->addItem(entityId, PageItem::Entity);
        qCDebug(lcPage()) << "Activity added" << entityId << m_id;
    }
}

void Page::removeActivity(QString entityId) {
    if (m_activities->contains(entityId)) {
        m_activities->removeItem(entityId);
        qCDebug(lcPage()) << "Activity removed" << entityId << m_id;
    }
}

}  // namespace ui
}  // namespace uc
