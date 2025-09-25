// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "util.h"
#include "logging.h"

namespace uc {

Util::Util(QObject *parent) : QObject(parent) {}

QString Util::FirstToUpper(const QString &str) {
    if (str.isEmpty()) {
        return QString();
    }

    QString tmp = str.toLower();
    tmp[0] = str[0].toUpper();
    return tmp;
}

QStringList Util::FirstToUpperList(const QStringList &list) {
    QStringList returnList;

    for (QStringList::const_iterator i = list.constBegin(); i != list.constEnd(); i++) {
        returnList.append(FirstToUpper(*i));
    }

    return returnList;
}

bool Util::FloatCompare(float f1, float f2) {
    static constexpr auto epsilon = 1.0e-05f;

    if (qAbs(f1 - f2) <= epsilon) {
        return true;
    }

    return qAbs(f1 - f2) <= epsilon * qMax(qAbs(f1), qAbs(f2));
}

QPixmap Util::generateQrCode(const QString &message, int size) {
    qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(message.toUtf8().constData(), qrcodegen::QrCode::Ecc::LOW);

    int qrSize = qr.getSize();

    QPixmap   pixmap(qrSize, qrSize);
    QPainter *painter = new QPainter(&pixmap);
    painter->setRenderHint(QPainter::Antialiasing);

    for (int y = 0; y < qrSize; y++) {
        for (int x = 0; x < qrSize; x++) {
            int color = qr.getModule(x, y);
            painter->setPen(color == 1 ? QColor("black") : QColor("#d0d0d0"));
            painter->drawPoint(x, y);
        }
    }

    delete painter;
    pixmap = pixmap.scaled(size, size);

    return pixmap;
}

QString Util::getDefaultCountryLocale(const QString &locale) {
    QString baseLang = locale.section('_', 0, 0);
    return baseLang + "_" + baseLang.toUpper();
}

QString Util::getLanguageString(QVariantMap map, const QString &language, QString fallback) {
    // see unit tests in /test/common/test_util.cpp
    // log is too verbose with many entities
    bool debug = false;
    if (debug) {
        qCDebug(lcI18n()) << "get language" << language << "in:" << map;
    }

    // if no language text is defined use provided default
    if (map.isEmpty()) {
        if (debug) {
            qCInfo(lcI18n()) << "empty language map!";
        }
        return fallback;
    }

    // direct match first, e.g. `de_DE`
    if (map.contains(language)) {
        QString text = map.value(language).toString();
        if (!text.isEmpty()) {
            if (debug) {
                qCDebug(lcI18n()) << "direct language match:" <<  text;
            }
            return text;
        }
    }

    // if not found try language fallback without country variant, e.g. `de`
    QString baseLang = language.section('_', 0, 0);
    if (!baseLang.isEmpty() && map.contains(baseLang)) {
        QString text = map.value(baseLang).toString();
        if (!text.isEmpty()) {
            if (debug) {
                qCDebug(lcI18n()) << "base language match:" << text;
            }
            return text;
        }
    }

    // search for a default-country match: fr_## -> fr_FR, it_## -> it_IT etc.
    // This is not a universal i18n logic, but works for languages we support.
    QString defaultLang = Util::getDefaultCountryLocale(language);
    if (!defaultLang.isEmpty() && map.contains(defaultLang)) {
        QString text = map.value(defaultLang).toString();
        if (!text.isEmpty()) {
            if (debug) {
                qCDebug(lcI18n()) << "default country language match:" << text;
            }
            return text;
        }
    }

    // try first non-empty country variant with same base language code (sorted by language key)
    // Note: "With QMap, the items are always sorted by key. ... The items are traversed in ascending key order."
    for (QVariantMap::const_iterator i = map.constBegin(); i != map.constEnd(); i++) {
        if (i.key().startsWith(baseLang)) {
            // special handling for the Swiss: one-way fallback to German, but NOT the other way around
            if (i.key() == "de_CH") {
                continue;
            }

            QString text = i.value().toString();
            if (text.isEmpty()) {
                continue;
            }
            if (debug) {
                qCDebug(lcI18n()) << "country language match:" <<  text;
            }
            return text;
        }
    }

    // fallback to English
    baseLang = "en";
    if (map.contains(baseLang)) {
        QString text = map.value(baseLang).toString();
        if (!text.isEmpty()) {
            if (debug) {
                qCDebug(lcI18n()) << "en language fallback:" <<  text;
            }
            return text;
        }
    }

    // pick first non-empty English variant (sorted by language key), e.g. `en_UK`. (QMap is sorted by key).
    for (QVariantMap::const_iterator i = map.constBegin(); i != map.constEnd(); i++) {
        if (i.key().startsWith(baseLang)) {
            QString text = i.value().toString();
            if (text.isEmpty()) {
                continue;
            }
            if (debug) {
                qCDebug(lcI18n()) << "en country language fallback:" <<  text;
            }
            return text;
        }
    }

    // final fallback: return first non-empty language entry
    for (QVariantMap::const_iterator i = map.constBegin(); i != map.constEnd(); i++) {
        QString text = i.value().toString();
        if (text.isEmpty()) {
            continue;
        }
        if (debug) {
            qCDebug(lcI18n()) << "final language fallback:" << text;
        }
        return text;
    }

    // nothing found
    return fallback;
}

}  // namespace uc
