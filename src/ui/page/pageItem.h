// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QVariant>

#include "../../util.h"

namespace uc {
namespace ui {

/**
 * @brief An item on a page, can be entity or group
 */
class PageItem : public QObject {
    Q_GADGET

 public:
    enum Type { Entity = 0, Group = 1 };
    Q_ENUM(Type)

    explicit PageItem(const QString& id, Type type, QObject* parent);
    ~PageItem();

    QString pageItemId() const { return m_id; }
    Type    pageItemType() const { return m_type; }

    static Type typeFromString(const QString& key, bool* ok = nullptr) {
        return Util::convertStringToEnum<Type>(key, ok);
    }

    static QString typeToString(Type value) { return Util::convertEnumToString<Type>(value).toLower(); }

 private:
    QString m_id;
    Type    m_type;
};

}  // namespace ui
}  // namespace uc
