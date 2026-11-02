// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
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

}  // namespace ValueFormat

}  // namespace entity
}  // namespace ui
}  // namespace uc
