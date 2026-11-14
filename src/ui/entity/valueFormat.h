// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QLocale>
#include <QString>
#include <QTimeZone>
#include <QVariant>

namespace uc {
namespace ui {
namespace entity {

/**
 * Display format of the numeric entity values: sensor values and number entities.
 *
 * The value is only rounded for display, the entity keeps the reported value.
 *
 * @see https://github.com/unfoldedcircle/core-api/blob/main/doc/ui-widgets.md#value-formatting
 */
namespace ValueFormat {

// Largest number of decimal places an entity option or user override may ask for.
constexpr int MAX_DECIMALS = 6;

/**
 * @brief The number of decimal places of a `decimals` entity option or user override, -1 if it is missing or not an
 * integer in 0..MAX_DECIMALS.
 */
int decimalsOption(const QVariant &value);

/**
 * @brief The number of decimal places a value is written with, at most MAX_DECIMALS.
 *
 * Floating point artifacts are ignored: 0.1 and 1.499999999999997 have 1 decimal place.
 */
int decimalPlaces(double value);

/**
 * @brief The value with exactly the given number of decimal places, in the current locale.
 *
 * The number of decimal places is clamped to 0..MAX_DECIMALS. A value that rounds to zero has no minus sign.
 */
QString fixed(double value, int decimals);

/**
 * @brief The value with at most 2 decimal places, trailing zeros dropped: the format of a sensor value without the
 * decimals option. 21.40 is shown as 21.4, 1.499999999999997 as 1.5.
 */
QString compact(double value);

/**
 * @brief A duration in days (`d`), hours (`h`) or minutes (`min`) as a time span in the unit of the value and the
 * next smaller unit: 1.5 h is shown as "1h 30m".
 *
 * Same logic as the Home Assistant frontend. With decimals >= 0, the value is rounded to that many decimal places
 * first. The fraction is rounded down to the next smaller unit, which is left out if it is zero. The value is not
 * converted to a larger unit: 93 h is shown as "93h".
 *
 * @return the time span, or an empty text if the value is not shown as a time span: any other unit, a value that is
 * not finite, and a negative value with a fraction.
 * @see https://github.com/unfoldedcircle/core-api/blob/main/doc/entities/entity_sensor.md#duration-format
 */
QString timeSpan(double value, const QString& unit, int decimals);

/**
 * @brief A point in time in RFC 3339 format as short date and time of the locale, in the given time zone.
 *
 * The time is shown as in the status bar, in the 12 or 24 hour format and without seconds. An invalid time zone
 * shows the system's local time.
 *
 * @return the date and time, or an empty text if the text is not a date and time with a UTC offset.
 */
QString dateTime(const QString& text, const QTimeZone& timeZone, bool clock24h, const QLocale& locale);

/**
 * @brief A calendar date in ISO 8601 format (YYYY-MM-DD) as short date of the locale.
 *
 * @return the date, or an empty text if the text is not a date.
 */
QString date(const QString& text, const QLocale& locale);

}  // namespace ValueFormat

}  // namespace entity
}  // namespace ui
}  // namespace uc
