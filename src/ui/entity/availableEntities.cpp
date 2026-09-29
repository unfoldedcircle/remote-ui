// Copyright (c) 2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#include "availableEntities.h"

#include "../../logging.h"
#include "../../util.h"
#include "entityController.h"

namespace uc {
namespace ui {

AvailableEntities::AvailableEntities(core::Api *core, QObject *parent) : Entities(core, parent) {
    QObject::connect(this, &AvailableEntities::filterChanged, this, &AvailableEntities::onFilterChanged);
}

void AvailableEntities::init(const QString &integrationId) {
    qCDebug(lcEntities()) << "Init" << this;
    clear();
    m_filter = core::AvailableEntitiesFilter();

    if (!integrationId.isEmpty()) {
        m_filter.integrationId = integrationId;
    }

    // opening the list is the moment a fresh list is wanted: let the core fetch the entities from the
    // integration again. Every other load reuses what the core already has.
    loadFromCore(DEFAULT_LIMIT, 1, true);
}

void AvailableEntities::search(const QString &searchString) {
    qCDebug(lcEntities()) << "Search:" << searchString;
    m_filter.textSearch = searchString;
    emit filterChanged();
}

void AvailableEntities::setIntegrationIds(const QStringList &integrationIds) {
    m_filter.integrationId = integrationIds[0];
    emit filterChanged();
}

void AvailableEntities::removeIntegrationIds(const QStringList &integrationIds) {
    Q_UNUSED(integrationIds)
    m_filter.integrationId.clear();
    emit filterChanged();
}

void AvailableEntities::clearIntegrationIds() {
    m_filter.integrationId.clear();
    emit filterChanged();
}

void AvailableEntities::setEntityType(int type) {
    entity::Base::Type typeEnum = static_cast<entity::Base::Type>(type);
    QString            strType = Util::convertEnumToString(typeEnum).toLower();
    m_filter.entityTypes.append(strType);
    emit filterChanged();
}

void AvailableEntities::removeEntityType(int type) {
    entity::Base::Type typeEnum = static_cast<entity::Base::Type>(type);
    QString            strType = Util::convertEnumToString(typeEnum).toLower();
    m_filter.entityTypes.removeOne(strType);
    emit filterChanged();
}

bool AvailableEntities::containsEntityType(int type) {
    entity::Base::Type typeEnum = static_cast<entity::Base::Type>(type);
    QString            strType = Util::convertEnumToString(typeEnum).toLower();
    if (strType.isEmpty()) {
        return false;
    }
    return m_filter.entityTypes.contains(strType);
}

void AvailableEntities::cleanEntityTypes() {
    m_filter.entityTypes.clear();
    emit filterChanged();
}

void AvailableEntities::loadMore() {
    qCDebug(lcEntities()) << "Load more";
    if (canLoadMore()) {
        loadFromCore(m_limit, m_lastPageLoaded + 1);
    }
}

void AvailableEntities::loadFromCore(int limit, int page) {
    loadFromCore(limit, page, false);
}

void AvailableEntities::loadFromCore(int limit, int page, bool forceReload) {
    int id = m_core->getAvailableEntities(limit, page, forceReload, m_filter);
    setActiveRequest(id);

    m_core->onResponseWithErrorResult(
        id, &core::Api::respAvailableEntities,
        [=](QList<core::Entity> entities, int count, int responseLimit, int page) {
            Q_UNUSED(responseLimit)  // number of items in this page, not the page size
            // success
            if (isStaleResponse(id)) {
                return;
            }

            qCDebug(lcEntities()) << "Available entities:" << count << "page:" << page << "limit:" << limit;

            setCount(count);

            if (count > 0) {
                m_totalItems = count;
                if (m_limit == 0) {
                    m_limit = limit;
                    m_totalPages = Util::pageCount(count, limit);
                }
                m_lastPageLoaded = page;

                if (entities.size() > 0) {
                    for (const auto &entity : entities) {
                        entity::Base *obj = EntityController::createEntityObject(
                            entity.type, entity.id, entity.name, entity.icon, entity.area, entity.deviceClass,
                            entity.features, entity.options, entity.enabled, entity.attributes, entity.integrationId,
                            this);

                        if (obj != nullptr) {
                            add(obj);
                        }
                    }
                }
            }

            emit entitiesLoaded(count);
        },
        [=](int code, QString message) {
            // fail
            if (isStaleResponse(id)) {
                return;
            }

            qCWarning(lcEntities()) << "Cannot get available entities" << code << message;
        });
}

void AvailableEntities::onFilterChanged() {
    qCDebug(lcEntities()) << "Filter changed, updating list" << m_filter.entityTypes << m_filter.integrationId;
    clear();
    loadFromCore();

    if (m_filter.entityTypes.isEmpty()) {
        m_filtered = false;
        emit filteredChanged();
    } else {
        m_filtered = true;
        emit filteredChanged();
    }
}

}  // namespace ui
}  // namespace uc
