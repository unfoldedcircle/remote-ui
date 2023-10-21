// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QUuid>

namespace uc {
namespace ui {

class NotificationItem : public QObject {
    Q_OBJECT

 public:
    explicit NotificationItem(const QString &id, QDateTime timestamp, const QString &title, const QString &message,
                              const QString &icon = QString(), void (*action)(QVariant) = nullptr,
                              QVariant param = QVariant(), const QString &actionLabel = QString(), bool warning = false,
                              QObject *parent = nullptr);
    ~NotificationItem();

    Q_INVOKABLE QString itemKey() const { return m_id; }
    QDateTime           itemTimestamp() const { return m_timestamp; }
    Q_INVOKABLE QString itemIcon() const { return m_icon; }
    Q_INVOKABLE QString itemTitle() const { return m_title; }
    Q_INVOKABLE QString itemMessage() const { return m_message; }
    Q_INVOKABLE QString itemActionLabel() const { return m_actionLabel; }
    Q_INVOKABLE bool    itemWarning() const { return m_warning; }

    Q_INVOKABLE void action();

 private:
    QString   m_id;
    QDateTime m_timestamp;
    QString   m_icon;
    QString   m_title;
    QString   m_message;
    void      (*m_action)(QVariant);
    QVariant  m_param;
    QString   m_actionLabel;
    bool      m_warning;
};

//============================================================================================================================================//

class NotificationsModel : public QAbstractListModel {
    Q_OBJECT

 public:
    enum SearchRoles {
        KeyRole = Qt::UserRole + 1,
        TimestampRole,
        IconRole,
        TitleRole,
        MessageRole,
        ActionLabeRole,
        WarningRole
    };

    explicit NotificationsModel(QObject *parent = nullptr);
    ~NotificationsModel() = default;

 public:
    // Q_PROPERTY methods
    int count() const;

 public slots:
    // Q_PROPERTY methods
    void setCount(int count);

 public:
    // QQAbstractListModel overrides and required emthods
    int                    rowCount(const QModelIndex &parent = QModelIndex()) const override;
    bool                   removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    QVariant               data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

 public:
    // Custom methods
    void append(NotificationItem *o);
    void clear();

    QModelIndex getModelIndexByKey(const QString &key);

    NotificationItem *get(const QString &key);
    NotificationItem *get(int row);

    void removeItem(const QString &key);
    void removeItem(int row);

 signals:
    void countChanged(int count);

 private:
    int                       m_count;
    QList<NotificationItem *> m_data;
};

//============================================================================================================================================//

class Notification : public QObject {
    Q_OBJECT

    Q_PROPERTY(QAbstractListModel *model READ getModel CONSTANT)

 public:
    explicit Notification(QObject *parent = nullptr);
    ~Notification();

    QAbstractListModel *getModel() { return &m_notifications; }

    Q_INVOKABLE void remove(const QString &id);
    Q_INVOKABLE void clearAll();

 public:
    static void createNotification(const QString &message, bool warning = false);
    static void createActionableNotification(const QString &title, const QString &message,
                                             const QString &icon = QString(), void (*action)(QVariant) = nullptr,
                                             QVariant param = QVariant(), const QString &actionLabel = QString());
    static void createActionableWarningNotification(const QString &title, const QString &message,
                                                    const QString &icon = QString(), void (*action)(QVariant) = nullptr,
                                                    QVariant       param = QVariant(),
                                                    const QString &actionLabel = QString());

    static QObject *qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine);

 signals:
    void notificationCreated(QString message, bool warning);
    void actionableNotificationCreated(QObject *notification);

 private:
    static Notification *s_instance;

    NotificationsModel m_notifications;
};
}  // namespace ui
}  // namespace uc
