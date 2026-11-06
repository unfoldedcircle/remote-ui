// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "valueFormat.h"

#include <QLocale>
#include <QtMath>
#include <cmath>

namespace uc {
namespace ui {
namespace entity {
namespace ValueFormat {

int decimalsOption(const QVariant &value) {
    bool         ok = false;
    const double decimals = value.toDouble(&ok);
    if (!ok || decimals != std::floor(decimals) || decimals < 0 || decimals > MAX_DECIMALS) {
        return -1;
    }
    return static_cast<int>(decimals);
}

int decimalPlaces(double value) {
    if (!qIsFinite(value)) {
        return 0;
    }

    for (int decimals = 0; decimals < MAX_DECIMALS; decimals++) {
        const double scaled = value * qPow(10, decimals);
        // relative tolerance: a binary double rarely hits a decimal fraction exactly
        if (qAbs(scaled - std::round(scaled)) <= 1e-9 * qMax(1.0, qAbs(scaled))) {
            return decimals;
        }
    }
    return MAX_DECIMALS;
}

QString fixed(double value, int decimals) {
    decimals = qBound(0, decimals, MAX_DECIMALS);

    const double factor = qPow(10, decimals);
    double       rounded = std::round(value * factor) / factor;
    if (rounded == 0) {
        // -0.04 with 1 decimal place is 0.0, not -0.0
        rounded = 0;
    }

    QLocale locale;
    // values are shown in narrow widgets and tiles
    locale.setNumberOptions(QLocale::OmitGroupSeparator);
    return locale.toString(rounded, 'f', decimals);
}

QString compact(double value) {
    QString text = fixed(value, 2);

    const QChar decimalPoint = QLocale().decimalPoint();
    if (text.contains(decimalPoint)) {
        while (text.endsWith(QLatin1Char('0'))) {
            text.chop(1);
        }
        if (text.endsWith(decimalPoint)) {
            text.chop(1);
        }
    }
    return text;
}

}  // namespace ValueFormat
}  // namespace entity
}  // namespace ui
}  // namespace uc
