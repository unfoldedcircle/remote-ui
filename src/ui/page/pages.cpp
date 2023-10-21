// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pages.h"

#include "../../logging.h"

namespace uc {
namespace ui {

Pages::Pages(QObject *parent) : QAbstractListModel(parent), m_count(0) {}

//============================================================================================================================================//
// Q_PROPERTY methods

int Pages::count() const {
    //    return m_count;
    return m_data.size();
}

void Pages::setCount(int count) {
    if (m_count == count) {
        return;
    }

    m_count = count;
    emit countChanged(m_count);
}

//============================================================================================================================================//
// QQAbstractListModel overrides and required emthods

int Pages::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return m_data.size();
}

bool Pages::removeRows(int row, int count, const QModelIndex &parent) {
    beginRemoveRows(parent, row, row + count - 1);
    for (int i = row + count - 1; i >= row; i--) {
        m_data.at(i)->deleteLater();
        m_data.removeAt(i);
    }
    endRemoveRows();
    return true;
}

QVariant Pages::data(const QModelIndex &index, int role) const {
    if (index.row() < 0 || index.row() >= m_data.count()) {
        return QVariant();
    }
    const Page *item = m_data[index.row()];
    switch (role) {
        case KeyRole:
            return item->pageId();
        case TitleRole:
            return item->pageName();
        case ImageRole:
            return item->pageImage();
        case DataRole:
            return QVariant::fromValue(item->pageItems());
        case ActivitiesRole:
            return QVariant::fromValue(item->pageActivities());
    }
    return QVariant();
}

bool Pages::setData(const QModelIndex &index, const QVariant &value, int role) {
    bool ret = false;

    Page *page = m_data[index.row()];

    switch (role) {
        case TitleRole:
            page->setItemTitle(value.toString());
            ret = true;
            break;
        case ImageRole:
            page->setItemImage(value.toString());
            ret = true;
            break;
    }

    if (ret) {
        emit dataChanged(index, index);
    }

    return ret;
}

QHash<int, QByteArray> Pages::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[KeyRole] = "pageId";
    roles[TitleRole] = "pageName";
    roles[ImageRole] = "pageImage";
    roles[DataRole] = "pageItems";
    roles[ActivitiesRole] = "pageActivities";
    return roles;
}

//============================================================================================================================================//

void Pages::append(Page *o) {
    int i = m_data.count();
    beginInsertRows(QModelIndex(), i, i);
    m_data.append(o);

    emit countChanged(count());
    endInsertRows();
}

void Pages::clear() {
    for (int i = 0; i < m_data.count(); i++) {
        m_data[i]->deleteLater();
    }

    beginResetModel();
    m_data.clear();
    endResetModel();
    emit countChanged(count());
}

QModelIndex Pages::getModelIndexByKey(const QString &key) {
    QModelIndex idx;

    for (int i = 0; i < m_data.count(); i++) {
        if (m_data[i]->pageId() == key) {
            idx = index(i, 0);
            break;
        }
    }

    return idx;
}

Page *Pages::getPage(const QString &key) {
    return m_data[getModelIndexByKey(key).row()];
}

Page *Pages::getPage(int row) {
    return m_data[row];
}

void Pages::removeItem(const QString &key) {
    removeItem(getModelIndexByKey(key).row());
}

void Pages::removeItem(int row) {
    removeRows(row, 1, QModelIndex());
    emit countChanged(count());
}

void Pages::updatePageName(const QString &key, const QString &name) {
    auto i = getModelIndexByKey(key);
    setData(i, name, TitleRole);
    emit dataChanged(i, i);
}

void Pages::updatePageImage(const QString &key, const QString &image) {
    auto i = getModelIndexByKey(key);
    setData(i, image, ImageRole);
    emit dataChanged(i, i);
}

//============================================================================================================================================//

void Pages::swapData(int from, int to) {
    qCDebug(lcPage()) << "Swap data from" << from << "to" << to;

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

QObject *Pages::get(const QString &key) {
    return m_data[getModelIndexByKey(key).row()];
}

void Pages::onEntityDeleted(QString entityId) {
    for (int i = 0; i < m_data.count(); i++) {
        m_data[i]->removeEntity(entityId);
    }
}

void Pages::onGroupDeleted(QString groupId) {
    for (int i = 0; i < m_data.count(); i++) {
        m_data[i]->removeGroup(groupId);
    }
}

}  // namespace ui
}  // namespace uc
