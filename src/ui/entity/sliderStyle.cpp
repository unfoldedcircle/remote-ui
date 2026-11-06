// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sliderStyle.h"

#include <QtMath>
#include <climits>
#include <cmath>

#include "entity.h"
#include "valueFormat.h"

namespace uc {
namespace ui {
namespace entity {

namespace {

struct FeatureDefaults {
    Base::Type         type;
    const char        *feature;
    SliderStyle::Color color;          // the colour of `auto`
    const char        *gradientStart;  // colour roles of the feature gradient, nullptr: none
    const char        *gradientEnd;
    SliderStyle::Fill  fill;  // the fill of `auto`; center falls back to start unless min < 0 < max
};

// The per-feature defaults: the one place to change how a feature looks without a style in the item.
const FeatureDefaults FEATURE_DEFAULTS[] = {
    {Base::Type::Number, "value", SliderStyle::Neutral, nullptr, nullptr, SliderStyle::Center},
    {Base::Type::Light, "dim", SliderStyle::Neutral, "surfaceRaised", "textPrimary", SliderStyle::Start},
    {Base::Type::Light, "color_temperature", SliderStyle::Gradient, "sliderGradientCool", "sliderGradientWarm",
     SliderStyle::Cursor},
    {Base::Type::Media_player, "volume", SliderStyle::Neutral, nullptr, nullptr, SliderStyle::Start},
    {Base::Type::Cover, "position", SliderStyle::Neutral, nullptr, nullptr, SliderStyle::Start},
    {Base::Type::Cover, "tilt_position", SliderStyle::Neutral, nullptr, nullptr, SliderStyle::Start},
    {Base::Type::Climate, "target_temperature", SliderStyle::Neutral, "sliderGradientCool", "sliderGradientHeat",
     SliderStyle::Start},
};

// an unknown feature: neutral, filled from the start
const FeatureDefaults UNKNOWN_FEATURE = {Base::Type::Unsupported, "", SliderStyle::Neutral, nullptr, nullptr,
                                         SliderStyle::Start};

const FeatureDefaults &defaults(int entityType, const QString &feature) {
    for (const auto &entry : FEATURE_DEFAULTS) {
        if (entry.type == entityType && feature == QLatin1String(entry.feature)) {
            return entry;
        }
    }
    return UNKNOWN_FEATURE;
}

double relativeLuminance(const QColor &color) {
    auto channel = [](double value) {
        return value <= 0.04045 ? value / 12.92 : std::pow((value + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * channel(color.redF()) + 0.7152 * channel(color.greenF()) + 0.0722 * channel(color.blueF());
}

}  // namespace

SliderStyle::SliderStyle(QObject *parent) : QObject(parent) {}

int SliderStyle::style(const QString &value) {
    if (value == QLatin1String("bar")) {
        return Bar;
    }
    if (value == QLatin1String("buttons")) {
        return Buttons;
    }
    return Default;
}

int SliderStyle::color(int entityType, const QString &feature, const QString &value, bool entityColorKnown) {
    const FeatureDefaults &entry = defaults(entityType, feature);

    if (value == QLatin1String("neutral")) {
        return Neutral;
    }
    if (value == QLatin1String("entity") && entityType == Base::Type::Light && feature == QLatin1String("dim")) {
        return entityColorKnown ? Entity : Neutral;
    }
    if (value == QLatin1String("gradient")) {
        return entry.gradientStart ? Gradient : Neutral;
    }
    return entry.color;
}

int SliderStyle::fill(int entityType, const QString &feature, const QString &value, double min, double max) {
    Fill fill = defaults(entityType, feature).fill;

    if (value == QLatin1String("start")) {
        fill = Start;
    } else if (value == QLatin1String("center")) {
        fill = Center;
    } else if (value == QLatin1String("cursor")) {
        fill = Cursor;
    }

    if (fill == Center && !(min < 0 && max > 0)) {
        return Start;
    }
    return fill;
}

QStringList SliderStyle::gradient(int entityType, const QString &feature) {
    const FeatureDefaults &entry = defaults(entityType, feature);
    if (!entry.gradientStart) {
        return {};
    }
    return {QLatin1String(entry.gradientStart), QLatin1String(entry.gradientEnd)};
}

double SliderStyle::dragValue(double startValue, double distance, double length, double min, double max) {
    if (length <= 0 || max <= min) {
        return qBound(min, startValue, qMax(min, max));
    }
    return qBound(min, startValue + distance / length * (max - min), max);
}

double SliderStyle::snap(double value, double min, double max, double step) {
    if (max <= min) {
        return min;
    }

    double snapped = qBound(min, value, max);
    if (step > 0) {
        const double nearest = min + std::round((snapped - min) / step) * step;
        // the last step may be shorter: max is a position too
        snapped = (nearest > max || max - snapped < qAbs(snapped - nearest)) ? max : nearest;
    }

    const int    decimals = qMax(ValueFormat::decimalPlaces(step), ValueFormat::decimalPlaces(min));
    const double scale = qPow(10, decimals);
    return std::round(snapped * scale) / scale;
}

int SliderStyle::positions(double min, double max, double step) {
    if (max <= min) {
        return 1;
    }
    if (step <= 0) {
        return INT_MAX;
    }
    // max is a position even if the last step is shorter; the tolerance absorbs floating point artifacts
    return static_cast<int>(std::ceil((max - min) / step - 1e-9)) + 1;
}

bool SliderStyle::discrete(double min, double max, double step) {
    return positions(min, max, step) <= MAX_DISCRETE_POSITIONS;
}

QColor SliderStyle::mix(const QColor &start, const QColor &end, double at) {
    at = qBound(0.0, at, 1.0);
    return QColor::fromRgbF(start.redF() + (end.redF() - start.redF()) * at,
                            start.greenF() + (end.greenF() - start.greenF()) * at,
                            start.blueF() + (end.blueF() - start.blueF()) * at);
}

QColor SliderStyle::contrastColor(const QColor &background, const QColor &first, const QColor &second) {
    return contrastRatio(background, first) >= contrastRatio(background, second) ? first : second;
}

double SliderStyle::contrastRatio(const QColor &first, const QColor &second) {
    const double a = relativeLuminance(first);
    const double b = relativeLuminance(second);
    return (qMax(a, b) + 0.05) / (qMin(a, b) + 0.05);
}

}  // namespace entity
}  // namespace ui
}  // namespace uc
