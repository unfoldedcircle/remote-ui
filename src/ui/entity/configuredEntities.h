// Copyright (c) 2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>

#include "entities.h"

namespace uc {
namespace ui {

class ConfiguredEntities : public Entities {
    Q_OBJECT

 public:
    explicit ConfiguredEntities(core::Api* core, QObject* parent = nullptr);
    ~ConfiguredEntities() {}

    Q_INVOKABLE void init(const QString& integrationId = QString()) override;

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
    core::EntityFilter m_filter;

    void loadFromCore(int limit = 100, int page = 1) override;

 private slots:
    void onFilterChanged() override;
};

}  // namespace ui
}  // namespace uc
