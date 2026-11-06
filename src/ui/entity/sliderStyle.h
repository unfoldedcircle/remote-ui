// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QColor>
#include <QObject>
#include <QStringList>

namespace uc {
namespace ui {
namespace entity {

/**
 * The look of a slider item on an activity page: its `style`, `color` and `fill`, resolved for the entity feature
 * from the per-feature defaults, and the drag arithmetic of SliderWidget.qml.
 *
 * Unknown or missing values are the default: style `default`, color and fill `auto`. The colours themselves are theme
 * roles of the Colors class; this class only names them.
 *
 * @see https://github.com/unfoldedcircle/core-api/blob/main/doc/ui-widgets.md#slider
 */
class SliderStyle : public QObject {
    Q_OBJECT

 public:
    enum Style { Default, Bar, Buttons };
    Q_ENUM(Style)

    // the resolved colour of the fill: never auto
    enum Color { Neutral, Entity, Gradient };
    Q_ENUM(Color)

    // the resolved fill: never auto
    enum Fill { Start, Center, Cursor };
    Q_ENUM(Fill)

    // a slider with at most this many positions is discrete: separators or ticks, snapped on release
    static constexpr int MAX_DISCRETE_POSITIONS = 10;

    explicit SliderStyle(QObject *parent = nullptr);

    /**
     * @brief The style of the `style` value: default, bar or buttons.
     */
    Q_INVOKABLE static int style(const QString &value);

    /**
     * @brief The colour of the fill for the `color` value.
     *
     * `auto` is the feature's default. `entity` is the light's colour, for the `dim` feature of a light with a known
     * colour only: another feature resolves like `auto`, a light without a known colour is neutral. `gradient` is the
     * feature's gradient, neutral if it has none.
     *
     * @param entityType a uc::ui::entity::Base::Type
     */
    Q_INVOKABLE static int color(int entityType, const QString &feature, const QString &value, bool entityColorKnown);

    /**
     * @brief The fill for the `fill` value. `auto` is the feature's default. `center` needs a range from below to
     * above zero, otherwise it is `start`.
     */
    Q_INVOKABLE static int fill(int entityType, const QString &feature, const QString &value, double min, double max);

    /**
     * @brief The colour roles of the feature's gradient from the start (minimum) to the end (maximum) of the range, or
     * an empty list if the feature has none.
     */
    Q_INVOKABLE static QStringList gradient(int entityType, const QString &feature);

    /**
     * @brief Relative drag: the value moved by the dragged distance, in proportion to the track length, within the
     * range. The value is not snapped to a step.
     */
    Q_INVOKABLE static double dragValue(double startValue, double distance, double length, double min, double max);

    /**
     * @brief The position nearest to the value: min plus a multiple of step, within the range, rounded to the
     * decimals of the step so 0.1 steps give 0.3 and not 0.30000000000000004.
     */
    Q_INVOKABLE static double snap(double value, double min, double max, double step);

    /**
     * @brief The number of positions from min to max in steps of step, the end included.
     */
    Q_INVOKABLE static int positions(double min, double max, double step);

    /**
     * @brief True if the range has at most MAX_DISCRETE_POSITIONS positions.
     */
    Q_INVOKABLE static bool discrete(double min, double max, double step);

    /**
     * @brief The colour at a fraction of 0..1 of a gradient from start to end.
     */
    Q_INVOKABLE static QColor mix(const QColor &start, const QColor &end, double at);

    /**
     * @brief Of the two colours, the one with the higher contrast on the background: the colour of text and icons
     * over a fill.
     */
    Q_INVOKABLE static QColor contrastColor(const QColor &background, const QColor &first, const QColor &second);

    /**
     * @brief The contrast ratio of two colours, 1 to 21, after WCAG 2.
     */
    static double contrastRatio(const QColor &first, const QColor &second);
};

}  // namespace entity
}  // namespace ui
}  // namespace uc
