// Copyright (c) 2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#include "configuredEntities.h"

#include "../../logging.h"
#include "entityController.h"

namespace uc {
namespace ui {

ConfiguredEntities::ConfiguredEntities(core::Api *core, QObject *parent) : Entities(core, parent) {
    QObject::connect(this, &ConfiguredEntities::filterChanged, this, &ConfiguredEntities::onFilterChanged);
}

void ConfiguredEntities::init(const QString &integrationId) {
    qCDebug(lcEntities()) << "Init" << this;
    clear();
    m_filter = core::EntityFilter();

    if (!integrationId.isEmpty()) {
        m_filter.integrationIds = QStringList() << integrationId;
    }
    loadFromCore();
}

void ConfiguredEntities::search(const QString &searchString) {
    qCDebug(lcEntities()) << "Search:" << searchString;
    m_filter.textSearch = searchString;
    emit filterChanged();
}

void ConfiguredEntities::setIntegrationIds(const QStringList &integrationIds) {
    m_filter.integrationIds = integrationIds;
    emit filterChanged();
}

void ConfiguredEntities::removeIntegrationIds(const QStringList &integrationIds) {
    QSet currentIds = QSet<QString>(m_filter.integrationIds.begin(), m_filter.integrationIds.end());
    QSet removeIds = QSet<QString>(integrationIds.begin(), integrationIds.end());

    currentIds.subtract(removeIds);
    m_filter.integrationIds = currentIds.values();
    emit filterChanged();
}

void ConfiguredEntities::clearIntegrationIds() {
    m_filter.integrationIds.clear();
    emit filterChanged();
}

void ConfiguredEntities::setEntityType(int type) {
    entity::Base::Type typeEnum = static_cast<entity::Base::Type>(type);
    QString            strType = Util::convertEnumToString(typeEnum).toLower();
    m_filter.entityTypes.append(strType);
    emit filterChanged();
}

void ConfiguredEntities::removeEntityType(int type) {
    entity::Base::Type typeEnum = static_cast<entity::Base::Type>(type);
    QString            strType = Util::convertEnumToString(typeEnum).toLower();
    m_filter.entityTypes.removeOne(strType);
    emit filterChanged();
}

bool ConfiguredEntities::containsEntityType(int type) {
    entity::Base::Type typeEnum = static_cast<entity::Base::Type>(type);
    QString            strType = Util::convertEnumToString(typeEnum).toLower();

    if (strType.isEmpty()) {
        return false;
    }
    return m_filter.entityTypes.contains(strType);
}

void ConfiguredEntities::cleanEntityTypes() {
    m_filter.entityTypes.clear();
    emit filterChanged();
}

void ConfiguredEntities::loadMore() {
    qCDebug(lcEntities()) << "Load more";
    if (canLoadMore()) {
        loadFromCore(m_limit, m_lastPageLoaded + 1);
    }
}

void ConfiguredEntities::loadFromCore(int limit, int page) {
    int id = m_core->getEntities(limit, page, m_filter);

    m_core->onResponseWithErrorResult(
        id, &core::Api::respEntities,
        [=](QList<core::Entity> entities, int count, int limit, int page) {
            // success
            qCDebug(lcEntities()) << "Configured entities:" << count << "page:" << page << "limit:" << limit;

            setCount(count);

            if (count > 0) {
                m_totalItems = count;
                if (m_limit == 0) {
                    m_limit = limit;
                    m_totalPages = qCeil(static_cast<float>(count) / static_cast<float>(limit));
                }
                m_lastPageLoaded = page;

                if (entities.size() > 0) {
                    for (const auto &entity : entities) {
                        entity::Base *obj = EntityController::createEntityObject(
                            entity.type, entity.id, entity.name, entity.icon, entity.area, entity.deviceClass,
                            entity.features, entity.options, entity.enabled, entity.attributes, entity.integrationId,
                            this);
                        add(obj);
                    }
                }
            }

            emit entitiesLoaded(count);
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcEntities()) << "Cannot get configured entities" << code << message;
        });
}

void ConfiguredEntities::onFilterChanged() {
    qCDebug(lcEntities()) << "Filter changed, updating list" << m_filter.entityTypes << m_filter.integrationIds;
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
