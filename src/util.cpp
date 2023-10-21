// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "util.h"

#include "logging.h"

namespace uc {

Util::Util(QObject *parent) : QObject(parent) {}

QString Util::FirstToUpper(const QString &str) {
    QString tmp = str.toLower();
    tmp[0] = str[0].toUpper();
    return tmp;
}

QStringList Util::FirstToUpperList(const QStringList &list) {
    QStringList returnList;

    for (QStringList::const_iterator i = list.begin(); i != list.end(); i++) {
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

QString Util::getLanguageString(QVariantMap map, const QString &language) {
    if (map.isEmpty()) {
        return QString();
    }

    if (map.contains(language)) {
        return map.value(language).toString();
    }

    if (map.contains("en")) {
        return map.value("en").toString();
    }

    if (map.size() > 0) {
        return map.first().toString();
    }

    return QString();
}

}  // namespace uc
