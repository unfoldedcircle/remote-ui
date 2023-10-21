// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QObject>

#include "../../core/core.h"
#include "pageItem.h"

namespace uc {
namespace ui {

/**
 * @brief This is the model that contains the entities on a page
 * @see https://doc.qt.io/qt-5/qabstractitemmodel.html
 */
class PageItemList : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged)

 public:
    enum SearchRoles { KeyRole = Qt::UserRole + 1, TypeRole };

    explicit PageItemList(QObject* parent = nullptr);
    ~PageItemList() = default;

 public:
    // Q_PROPERTY methods
    int count() const;

 public slots:
    // Q_PROPERTY methods
    void setCount(int count);

 public:
    // QQAbstractListModel overrides and required emthods
    int      rowCount(const QModelIndex& parent = QModelIndex()) const override;
    bool     removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    // bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    QHash<int, QByteArray> roleNames() const override;

 public:
    // Custom methods
    void append(PageItem* item);
    void clear();

    PageItem* getPageItem(int row);
    PageItem* getPageItem(const QString& key);

    QModelIndex getModelIndexByKey(const QString& key);

 public:
    // QML accesible methods
    Q_INVOKABLE bool contains(const QString& key);
    Q_INVOKABLE void swapData(int from, int to);

    Q_INVOKABLE void addItem(const QString& key, PageItem::Type type);
    Q_INVOKABLE void removeItem(const QString& key);
    Q_INVOKABLE void removeItem(int row);

 signals:
    void countChanged(int count);

 private:
    int              m_count;
    QList<PageItem*> m_data;
};

/**
 * @brief This is a page that contains entities
 * It has an id(key), title, image and a model that contains the entities/groups
 */
class Page : public QObject {
    Q_OBJECT

 public:
    // TODO(zehnm) create a proper POD object with copy constructor etc
    explicit Page(const QString& key, const QString& name, const QString& image = QString(), QObject* parent = nullptr);

    ~Page();

    void init(const QList<core::PageItem>& items);

    QString       pageId() const { return m_id; }
    QString       pageName() const { return m_name; }
    QString       pageImage() const { return m_image; }
    PageItemList* pageItems() const { return m_items; }
    PageItemList* pageActivities() const { return m_activities; }

    void setItemKey(const QString& id) { m_id = id; }
    void setItemTitle(const QString& name) { m_name = name; }
    void setItemImage(const QString& image) { m_image = image; }

    PageItemList* m_items;

 public:
    // QML accesible methods
    Q_INVOKABLE void addEntity(const QString& entityId);
    Q_INVOKABLE void removeEntity(const QString& entityId);
    Q_INVOKABLE void addEntities(const QStringList& entities);

    Q_INVOKABLE void addGroup(const QString& groupId);
    void             removeGroup(const QString& groupId);
    Q_INVOKABLE void addGroups(const QStringList& groups);

    void removeEntities();

    void addActivity(QString entityId);
    void removeActivity(QString entityId);

 signals:
    void requestEntity(QString entityId);

 private:
    QString m_id;
    QString m_name;
    QString m_image;

    PageItemList* m_activities;
};

}  // namespace ui
}  // namespace uc
