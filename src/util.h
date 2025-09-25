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
    // REFACTOR why using a class if all member functions are static?
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

    /**
     * Get the default country locale for a given locale.
     *
     * The default country locale has the same country suffix as the language code. For example: `de_DE`, `fr_FR`, `it_IT`.
     * @param locale the language locale, either just a language identifier as `de` or a full identifier as `de_CH`.
     */
    static QString getDefaultCountryLocale(const QString &locale);

    /**
     * @brief Retrieve a language text from a language map for a given locale.
     * @details The provided fallback text is returned if the map is empty. Language retrieval logic:
     * 1. Try retrieving an exact language match first. E.g. `de_CH`.
     * 2. Then try without country specific variant only. E.g. `de`.
     * 3. Then try another country variant. If multiple variants are available, a random variant is
     *    returned. E.g. `de_AT`
     * 4. If the language is not available, the default English text with key `en` is returned.
     * 5. If an English text is missing, the first entry in the map is returned.
     * @param map: the language map with (language_key, language_text) entries.
     * @param language: the language locale, either just a language identifer as `de` or a full identifier as `de_CH`.
     * @param fallback: the default text if the language map is empty.
     * @return the found language text or given fallback text if map is empty.
     */
    static QString getLanguageString(QVariantMap map, const QString &language, QString fallback = "");
};

}  // namespace uc
