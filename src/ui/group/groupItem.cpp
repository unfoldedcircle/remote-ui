// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "groupItem.h"

#include "../../logging.h"

namespace uc {
namespace ui {

GroupItem::GroupItem(const QString &id, QObject *parent) : QObject(parent), m_id(id) {
    qCDebug(lcGroup()) << "Group item constructor:" << m_id;

    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
}

GroupItem::~GroupItem() {
    qCDebug(lcGroup()).noquote() << "Group item destructor:" << m_id;
}

}  // namespace ui
}  // namespace uc
