// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QVariant>

namespace uc {
namespace ui {

/**
 * @brief An item on a group
 */
class GroupItem : public QObject {
    Q_OBJECT

 public:
    explicit GroupItem(const QString& id, QObject* parent);
    ~GroupItem();

    QString groupItemId() const { return m_id; }

 private:
    QString m_id;
};

}  // namespace ui
}  // namespace uc
