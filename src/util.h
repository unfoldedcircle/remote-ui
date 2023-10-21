// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QColor>
#include <QMetaEnum>
#include <QObject>
#include <QPainter>
#include <QPixmap>
#include <QtMath>

#include "3rd-party/QR-Code-generator/cpp/qrcodegen.hpp"

namespace uc {

class Util : public QObject {
    Q_OBJECT
 public:
    explicit Util(QObject *parent = nullptr);

    static QString     FirstToUpper(const QString &str);
    static QStringList FirstToUpperList(const QStringList &list);

    static bool FloatCompare(float f1, float f2);

    template <class T>
    static T convertStringToEnum(const QString &enumString, bool *ok = nullptr) {
        return static_cast<T>(QMetaEnum::fromType<T>().keyToValue(enumString.toUtf8(), ok));
    }

    template <class T>
    static QString convertEnumToString(T value) {
        return QVariant::fromValue(value).toString();
    }

    static QPixmap generateQrCode(const QString &message, int size);

    static QString getLanguageString(QVariantMap map, const QString &language);
};

}  // namespace uc
