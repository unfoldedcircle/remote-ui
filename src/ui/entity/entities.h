// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QVariant>

#include "../../core/core.h"
#include "entity.h"

namespace uc {
namespace ui {

class Entities : public QAbstractListModel {
    Q_OBJECT

    Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged)
    Q_PROPERTY(bool allSelected READ getAllSelected NOTIFY allSelectedChanged)
    Q_PROPERTY(bool filtered READ getFiltered NOTIFY filteredChanged)

 public:
    enum SearchRoles { KeyRole = Qt::UserRole + 1, NameRole, IconRole, IntegrationRole, TypeRole, SelectedRole };

    explicit Entities(core::Api* core, QObject* parent = nullptr);
    ~Entities() = default;

    int                    rowCount(const QModelIndex& parent = QModelIndex()) const override;
    bool                   removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    QVariant               data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int  count() const;
    void setCount(int count);
    bool getAllSelected() { return m_allSelected; }
    bool getFiltered() { return m_filtered; }

    virtual void     init(const QString& integrationId) = 0;
    Q_INVOKABLE void clear();

    virtual void search(const QString& searchString) = 0;

    virtual void setIntegrationIds(const QStringList& integrationIds) = 0;
    virtual void removeIntegrationIds(const QStringList& integrationIds) = 0;
    virtual void clearIntegrationIds() = 0;

    virtual void setEntityType(int type) = 0;
    virtual void removeEntityType(int type) = 0;
    virtual bool containsEntityType(int type) = 0;
    virtual void cleanEntityTypes() = 0;

    Q_INVOKABLE bool canLoadMore();

    virtual void loadMore() = 0;

    Q_INVOKABLE void        selectAll();
    Q_INVOKABLE void        clearSelected();
    Q_INVOKABLE void        setSelected(const QString& entityId, bool value);
    Q_INVOKABLE QStringList getSelected();

 signals:
    void countChanged(int count);
    void allSelectedChanged();
    void filteredChanged();
    void filterChanged();
    void entitiesLoaded(int count);

 protected:
    core::Api*                    m_core;
    QHash<QString, entity::Base*> m_data;

    int m_count = 0;
    int m_totalPages = 0;
    int m_lastPageLoaded = 0;
    int m_totalItems = 0;
    int m_limit = 0;

    bool m_filtered = false;

    bool m_allSelected = false;

    void add(entity::Base* o);
    void remove(const QString& key);
    void remove(int row);

    bool contains(const QString& key);

 protected:
    QModelIndex getModelIndexByKey(const QString& key);

    virtual void loadFromCore(int limit = 100, int page = 1) = 0;

 protected slots:
    virtual void onFilterChanged() = 0;
};

}  // namespace ui
}  // namespace uc
