// Copyright (c) 2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#include "entitiesProxy.h"

#include "../../logging.h"
#include "entity.h"

namespace uc {
namespace ui {

EntitiesProxy::EntitiesProxy(QObject *parent) : QSortFilterProxyModel(parent) {}

void EntitiesProxy::setRoles(int integrationRole, int typeRole) {
    m_integrationRole = integrationRole;
    m_typeRole = typeRole;
}

void EntitiesProxy::setIntegrationFitler(const QString &integrationId) {
    m_integration = integrationId;
    invalidateFilter();
}

void EntitiesProxy::setTypeFilter(QList<entity::Base::Type> types) {
    m_types = types;
    invalidateFilter();
}

void EntitiesProxy::addTypeFilter(entity::Base::Type type) {
    m_types.append(type);
    invalidateFilter();
}

void EntitiesProxy::removeTypeFilter(entity::Base::Type type) {
    m_types.removeOne(type);
    invalidateFilter();
}

void EntitiesProxy::clearTypeFilter() {
    m_types.clear();
    invalidateFilter();
}

bool EntitiesProxy::hasTypeFilter(entity::Base::Type type) {
    return m_types.contains(type);
}

bool EntitiesProxy::isTypeFiltered() {
    return m_types.length() > 0;
}

bool EntitiesProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
    QModelIndex iconIndex = sourceModel()->index(sourceRow, 0, sourceParent);

    if (!iconIndex.isValid()) {
        return false;
    }

    int integrationMatch = -1;
    if (!m_integration.isEmpty()) {
        QString integrationValue = iconIndex.data(m_integrationRole).toString();
        integrationMatch = integrationValue.contains(m_integration);
    }

    int typeMatch = -1;
    if (m_types.length() != 0) {
        entity::Base::Type typeValue = iconIndex.data(m_typeRole).value<entity::Base::Type>();
        typeMatch = m_types.contains(typeValue);
    }

    return integrationMatch != 0 && typeMatch != 0;
}

}  // namespace ui
}  // namespace uc
