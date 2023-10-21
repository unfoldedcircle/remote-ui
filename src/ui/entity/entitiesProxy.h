// Copyright (c) 2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QSortFilterProxyModel>

#include "entity.h"

namespace uc {
namespace ui {

class EntitiesProxy : public QSortFilterProxyModel {
    Q_OBJECT

 public:
    explicit EntitiesProxy(QObject *parent = nullptr);

    void setRoles(int integrationRole, int typeRole);

    void setIntegrationFitler(const QString &integrationId);
    void setTypeFilter(QList<entity::Base::Type> types);
    void addTypeFilter(entity::Base::Type type);
    void removeTypeFilter(entity::Base::Type type);
    void clearTypeFilter();
    bool hasTypeFilter(entity::Base::Type type);
    bool isTypeFiltered();

 protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

 private:
    int m_integrationRole;
    int m_typeRole;

    QString                   m_integration;
    QList<entity::Base::Type> m_types;
};

}  // namespace ui
}  // namespace uc
