// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QVariant>

#include "page.h"

namespace uc {
namespace ui {

/**
 * @brief This model containts all pages that are shown on the screen
 * Every item in this model is a Page class
 * @see https://doc.qt.io/qt-5/qabstractitemmodel.html
 */
class Pages : public QAbstractListModel {
    Q_OBJECT

    Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged)

 public:
    enum SearchRoles { KeyRole = Qt::UserRole + 1, TitleRole, ImageRole, DataRole, ActivitiesRole };

    explicit Pages(QObject* parent = nullptr);
    ~Pages() = default;

 public:
    // Q_PROPERTY methods
    int count() const;

 public slots:
    // Q_PROPERTY methods
    void setCount(int count);

 public:
    // QQAbstractListModel overrides and required emthods
    int                    rowCount(const QModelIndex& parent = QModelIndex()) const override;
    bool                   removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    QVariant               data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool                   setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    QHash<int, QByteArray> roleNames() const override;

 public:
    // Custom methods
    void append(Page* o);
    void clear();

    QModelIndex getModelIndexByKey(const QString& key);

    Page* getPage(const QString& key);
    Page* getPage(int row);

    void removeItem(const QString& key);
    void removeItem(int row);

    void updatePageName(const QString& key, const QString& name);
    void updatePageImage(const QString& key, const QString& image);

 public:
    // QML accesible methods
    Q_INVOKABLE void     swapData(int from, int to);
    Q_INVOKABLE QObject* get(const QString& key);

 signals:
    void countChanged(int count);

 public slots:
    void onEntityDeleted(QString entityId);
    void onGroupDeleted(QString groupId);

 private:
    int          m_count;
    QList<Page*> m_data;
};

}  // namespace ui
}  // namespace uc
