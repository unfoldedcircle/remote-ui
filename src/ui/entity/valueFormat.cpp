// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "valueFormat.h"

#include <QCoreApplication>
#include <QDateTime>
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

QString timeSpan(double value, const QString& unit, int decimals) {
    enum Unit { Days, Hours, Minutes, Seconds };
    // short unit labels, joined without a space to the value as in "3d 21h"
    const auto label = [](Unit labelUnit, double amount) {
        const QString number = fixed(amount, 0);
        switch (labelUnit) {
            case Days:
                //: Time span of a sensor: number of days with a short unit label. %1 is the number. Example: 3d
                return QCoreApplication::translate("Time span", "%1d").arg(number);
            case Hours:
                //: Time span of a sensor: number of hours with a short unit label. %1 is the number. Example: 21h
                return QCoreApplication::translate("Time span", "%1h").arg(number);
            case Minutes:
                //: Time span of a sensor: number of minutes with a short unit label. %1 is the number. Example: 30m
                return QCoreApplication::translate("Time span", "%1m").arg(number);
            default:
                //: Time span of a sensor: number of seconds with a short unit label. %1 is the number. Example: 15s
                return QCoreApplication::translate("Time span", "%1s").arg(number);
        }
    };

    const bool seconds = unit == QLatin1String("s");
    Unit       first = Minutes;
    double     factor = 60;
    if (unit == QLatin1String("d")) {
        first = Days;
        factor = 24;
    } else if (unit == QLatin1String("h")) {
        first = Hours;
    } else if (unit != QLatin1String("min") && !seconds) {
        return QString();
    }
    if (!qIsFinite(value)) {
        return QString();
    }

    if (decimals >= 0) {
        // rounds a half up, as the reference implementation does
        const double scale = qPow(10, qMin(decimals, MAX_DECIMALS));
        value = std::floor(value * scale + 0.5) / scale;
    }

    if (seconds) {
        // not part of the reference implementation, which shows seconds as a number: the largest unit the value
        // fills and the next smaller one. A negative value is split like a positive one.
        const double amount = std::floor(qAbs(value));
        if (amount < SECONDS_SPLIT_LIMIT) {
            return QString();
        }
        static const double unitSeconds[] = {86400, 3600, 60, 1};
        first = amount >= unitSeconds[Days] ? Days : amount >= unitSeconds[Hours] ? Hours : Minutes;

        const double whole = std::floor(amount / unitSeconds[first]);
        const double rest = std::floor((amount - whole * unitSeconds[first]) / unitSeconds[first + 1]);
        QString      text = label(first, value < 0 ? -whole : whole);
        if (rest > 0) {
            text += QLatin1Char(' ') + label(static_cast<Unit>(first + 1), rest);
        }
        return text;
    }

    const double whole = std::floor(value);
    const double rest = std::floor((value - whole) * factor);
    if (whole < 0 && rest > 0) {
        // -1.5 h would be "-2h 30m": shown as a number
        return QString();
    }

    QString text = label(first, whole);
    if (rest > 0) {
        text += QLatin1Char(' ') + label(static_cast<Unit>(first + 1), rest);
    }
    return text;
}

QString dateTime(const QString& text, const QTimeZone& timeZone, bool clock24h, const QLocale& locale) {
    QDateTime dateTime = QDateTime::fromString(text, Qt::ISODateWithMs);
    // without a UTC offset the point in time is not known
    if (!dateTime.isValid() || dateTime.timeSpec() == Qt::LocalTime) {
        return QString();
    }

    dateTime = timeZone.isValid() ? dateTime.toTimeZone(timeZone) : dateTime.toLocalTime();
    return locale.toString(dateTime.date(), QLocale::ShortFormat) + QLatin1Char(' ') +
           locale.toString(dateTime.time(), clock24h ? QStringLiteral("hh:mm") : QStringLiteral("hh:mm a"));
}

QString date(const QString& text, const QLocale& locale) {
    const QDate date = QDate::fromString(text, Qt::ISODate);
    return date.isValid() ? locale.toString(date, QLocale::ShortFormat) : QString();
}

}  // namespace ValueFormat
}  // namespace entity
}  // namespace ui
}  // namespace uc
