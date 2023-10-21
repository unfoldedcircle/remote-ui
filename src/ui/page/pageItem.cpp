// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pageItem.h"

#include "../../logging.h"

namespace uc {
namespace ui {

PageItem::PageItem(const QString &id, Type type, QObject *parent) : QObject(parent), m_id(id), m_type(type) {
    qCDebug(lcPage()) << "Page item constructor:" << m_type << m_id;

    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
}

PageItem::~PageItem() {
    qCDebug(lcPage()).noquote() << "Page item destructor:" << m_id << m_type;
}

}  // namespace ui
}  // namespace uc
