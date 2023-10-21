// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notification.h"

#include "../logging.h"

namespace uc {
namespace ui {

NotificationItem::NotificationItem(const QString &id, QDateTime timestamp, const QString &title, const QString &message,
                                   const QString &icon, void (*action)(QVariant), QVariant param,
                                   const QString &actionLabel, bool warning, QObject *parent)
    : QObject(parent),
      m_id(id),
      m_timestamp(timestamp),
      m_icon(icon),
      m_title(title),
      m_message(message),
      m_action(action),
      m_param(param),
      m_actionLabel(actionLabel),
      m_warning(warning) {
    qCDebug(lcNotification()) << "Notification item constructor:" << m_id << m_title;
    QQmlEngine::setObjectOwnership(this,
                                   parent == nullptr ? QQmlEngine::JavaScriptOwnership : QQmlEngine::CppOwnership);
}

NotificationItem::~NotificationItem() {
    qCDebug(lcNotification()).noquote() << "Notification item destructor:" << m_id << m_title;
}

void NotificationItem::action() {
    if (m_action) {
        m_action(m_param);
        qCDebug(lcNotification()).noquote() << "Action executed";
    }
}

//============================================================================================================================================//

NotificationsModel::NotificationsModel(QObject *parent) : QAbstractListModel(parent), m_count(0) {}

int NotificationsModel::count() const {
    return m_data.size();
}

void NotificationsModel::setCount(int count) {
    if (m_count == count) {
        return;
    }

    m_count = count;
    emit countChanged(m_count);
}

int NotificationsModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return m_data.size();
}

bool NotificationsModel::removeRows(int row, int count, const QModelIndex &parent) {
    beginRemoveRows(parent, row, row + count - 1);
    for (int i = row + count - 1; i >= row; i--) {
        m_data.at(i)->deleteLater();
        m_data.removeAt(i);
    }
    endRemoveRows();
    return true;
}

QVariant NotificationsModel::data(const QModelIndex &index, int role) const {
    if (index.row() < 0 || index.row() >= m_data.count()) {
        return QVariant();
    }
    const NotificationItem *item = m_data[index.row()];
    switch (role) {
        case KeyRole:
            return item->itemKey();
        case TimestampRole:
            return item->itemTimestamp().toString("MMMM d hh:mm");
        case IconRole:
            return item->itemIcon();
        case TitleRole:
            return item->itemTitle();
        case MessageRole:
            return item->itemMessage();
        case ActionLabeRole:
            return item->itemActionLabel();
        case WarningRole:
            return item->itemWarning();
    }
    return QVariant();
}

QHash<int, QByteArray> NotificationsModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[KeyRole] = "itemKey";
    roles[TimestampRole] = "itemTimettamp";
    roles[IconRole] = "itemIcon";
    roles[TitleRole] = "itemTitle";
    roles[MessageRole] = "itemMessage";
    roles[ActionLabeRole] = "itemActionLabel";
    roles[WarningRole] = "itemWarning";
    return roles;
}

void NotificationsModel::append(NotificationItem *o) {
    //    int i = m_data.size();
    emit layoutAboutToBeChanged();

    beginInsertRows(QModelIndex(), 0, 0);
    m_data.append(o);

    emit countChanged(count());
    endInsertRows();

    emit layoutChanged();
}

void NotificationsModel::clear() {
    for (int i = 0; i < m_data.count(); i++) {
        m_data[i]->deleteLater();
    }

    beginResetModel();
    m_data.clear();
    endResetModel();
    emit countChanged(count());
}

QModelIndex NotificationsModel::getModelIndexByKey(const QString &key) {
    QModelIndex idx;

    for (int i = 0; i < m_data.count(); i++) {
        if (m_data[i]->itemKey() == key) {
            idx = index(i, 0);
            break;
        }
    }

    return idx;
}

NotificationItem *NotificationsModel::get(const QString &key) {
    return m_data[getModelIndexByKey(key).row()];
}

NotificationItem *NotificationsModel::get(int row) {
    return m_data[row];
}

void NotificationsModel::removeItem(const QString &key) {
    removeItem(getModelIndexByKey(key).row());
}

void NotificationsModel::removeItem(int row) {
    removeRows(row, 1, QModelIndex());
    emit countChanged(count());
}

//============================================================================================================================================//

Notification *Notification::s_instance = nullptr;

Notification::Notification(QObject *parent) : QObject(parent) {
    Q_ASSERT(s_instance == nullptr);
    s_instance = this;
}

Notification::~Notification() {
    s_instance = nullptr;
}

void Notification::remove(const QString &id) {
    qCDebug(lcNotification()) << "Remove notification" << id;
    s_instance->m_notifications.removeItem(id);
}

void Notification::clearAll() {
    s_instance->m_notifications.clear();
}

void Notification::createNotification(const QString &message, bool warning) {
    emit s_instance->notificationCreated(message, warning);
}

void Notification::createActionableWarningNotification(const QString &title, const QString &message,
                                                       const QString &icon, void (*action)(QVariant), QVariant param,
                                                       const QString &actionLabel) {
    NotificationItem *notificationItem =
        new NotificationItem(QUuid::createUuid().toString().replace("{", "").replace("}", ""),
                             QDateTime::currentDateTime(), title, message, icon, action, param, actionLabel, true);

    //    s_instance->m_notifications.append(notificationItem);
    emit s_instance->actionableNotificationCreated(notificationItem);
}

void Notification::createActionableNotification(const QString &title, const QString &message, const QString &icon,
                                                void (*action)(QVariant), QVariant param, const QString &actionLabel) {
    NotificationItem *notificationItem =
        new NotificationItem(QUuid::createUuid().toString().replace("{", "").replace("}", ""),
                             QDateTime::currentDateTime(), title, message, icon, action, param, actionLabel, false);

    //    s_instance->m_notifications.append(notificationItem);
    emit s_instance->actionableNotificationCreated(notificationItem);
}

QObject *Notification::qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(scriptEngine);

    QObject *obj = s_instance;
    engine->setObjectOwnership(obj, QQmlEngine::CppOwnership);

    return obj;
}

}  // namespace ui
}  // namespace uc
