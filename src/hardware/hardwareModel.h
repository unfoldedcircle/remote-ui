// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QMetaEnum>
#include <QObject>
#include <QVariant>

namespace uc {
namespace hw {

class HardwareModel : public QObject {
    Q_OBJECT

 public:
    enum Enum { DEV, YIO1, UCR2, UCR3 };
    Q_ENUM(Enum)

    static Enum fromString(const QString& key, bool* ok = nullptr) {
        return static_cast<Enum>(QMetaEnum::fromType<Enum>().keyToValue(key.toUtf8(), ok));
    }

    static QString toString(Enum value) { return QVariant::fromValue(value).toString(); }
};

}  // namespace hw
}  // namespace uc
