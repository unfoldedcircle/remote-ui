// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QObject>

#include "../../core/core.h"
#include "groupItem.h"

namespace uc {
namespace ui {

/**
 * @brief This is the model that contains the entities on a group
 */
class GroupItemList : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged)

 public:
    enum SearchRoles { KeyRole = Qt::UserRole + 1 };

    explicit GroupItemList(QObject* parent = nullptr);
    ~GroupItemList() = default;

    int count() const;

    /**
     * @brief add an item to the model
     * @param item: the GrouopItem object
     */
    void append(GroupItem* item);

    /**
     * @brief Returns the number of screen groups held in this model.
     * @see https://doc.qt.io/qt-5/qabstractitemmodel.html#rowCount
     */
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    /**
     * @brief Removes rows from the model
     * @see https://doc.qt.io/qt-5/qabstractitemmodel.html#removeRows
     */
    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;

    /**
     * @brief Returns the GroupItem property (key, title, image, items) stored under the given role for the item
     * referred to by the index
     * @see https://doc.qt.io/qt-5/qabstractitemmodel.html#data
     */
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    // bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

    /**
     * @brief Returns the defined role names for accessing GroupItem properties
     * @see https://doc.qt.io/qt-5/qabstractitemmodel.html#roleNames
     */
    QHash<int, QByteArray> roleNames() const override;

    /**
     * @brief clears the model
     */
    void clear();

    /**
     * @param row: row of the group
     * @return group item
     */
    GroupItem*             getGroupItem(int row);
    Q_INVOKABLE GroupItem* getGroupItem(const QString& key);

    QModelIndex getModelIndexByKey(const QString& key);

    Q_INVOKABLE bool contains(const QString& key);
    Q_INVOKABLE void addItem(const QString& key);
    Q_INVOKABLE void removeItem(const QString& key);
    Q_INVOKABLE void removeItem(int row);
    Q_INVOKABLE void swapData(int from, int to);

 public slots:
    void setCount(int count);

 signals:
    void countChanged(int count);

 private:
    int               m_count;
    QList<GroupItem*> m_data;
};

/**
 * @brief This is a group that contains entities
 */
class Group : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString name READ groupName NOTIFY nameChanged);
    Q_PROPERTY(QString icon READ groupIcon NOTIFY iconChanged);

 public:
    // TODO(zehnm) create a proper POD object with copy constructor etc
    explicit Group(const QString& id, const QString& profileId, const QString& name, const QString& icon = QString(),
                   QObject* parent = nullptr);

    ~Group();

    void init(const QStringList& items);

    void setIsOn(bool value);

    Q_INVOKABLE QString  groupId() const { return m_id; }
    QString              groupProfileId() const { return m_profileId; }
    Q_INVOKABLE QString  groupName() const { return m_name; }
    Q_INVOKABLE QString  groupIcon() const { return m_icon; }
    Q_INVOKABLE QObject* groupItems() const { return m_items; }

    Q_INVOKABLE QStringList getEntities();
    void                    clearEntities();

    void setItemKey(const QString& key) { m_id = key; }
    void setItemProfileId(const QString& id) { m_profileId = id; }
    void setItemName(const QString& title) {
        m_name = title;
        emit nameChanged();
    }
    void setItemIcon(const QString& image) {
        m_icon = image;
        emit iconChanged();
    }

    GroupItemList* m_items;

 public:
    // QML accesible methods
    Q_INVOKABLE void addEntity(const QString& entityId);
    Q_INVOKABLE void removeEntity(const QString& entityId);
    Q_INVOKABLE void addEntities(const QStringList& entities);

 signals:
    void nameChanged();
    void iconChanged();
    void requestEntity(QString entityId);

 private:
    QString m_id;
    QString m_profileId;
    QString m_name;
    QString m_icon;
};

}  // namespace ui
}  // namespace uc
