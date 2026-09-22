// Copyright (c) 2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>

#include "entities.h"

namespace uc {
namespace ui {

class AvailableEntities : public Entities {
    Q_OBJECT

 public:
    explicit AvailableEntities(core::Api* core, QObject* parent = nullptr);
    ~AvailableEntities() {}

    Q_INVOKABLE void init(const QString& integrationId) override;

    Q_INVOKABLE void search(const QString& searchString) override;

    Q_INVOKABLE void setIntegrationIds(const QStringList& integrationIds) override;
    Q_INVOKABLE void removeIntegrationIds(const QStringList& integrationIds) override;
    Q_INVOKABLE void clearIntegrationIds() override;

    Q_INVOKABLE void setEntityType(int type) override;
    Q_INVOKABLE void removeEntityType(int type) override;
    Q_INVOKABLE bool containsEntityType(int type) override;
    Q_INVOKABLE void cleanEntityTypes() override;

    Q_INVOKABLE void loadMore() override;

 private:
    static constexpr int DEFAULT_LIMIT = 100;

    core::AvailableEntitiesFilter m_filter;

    void loadFromCore(int limit = DEFAULT_LIMIT, int page = 1) override;

    /**
     * @brief Loads a page of available entities from the core.
     * @param forceReload asks the core to fetch the entities from the integration again. That is an
     * expensive round trip for the integration and is only wanted when the list is opened, not for
     * every keystroke of a search.
     */
    void loadFromCore(int limit, int page, bool forceReload);

 private slots:
    void onFilterChanged() override;
};

}  // namespace ui
}  // namespace uc
