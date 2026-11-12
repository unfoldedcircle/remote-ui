// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>
#include <cmath>

#include "ui/colors.h"

/**
 * The colour tokens of the design system (docs/design-system.md, ADR 0021). The palette is chosen against the raised
 * black of the Remote 3 LCD, so text keeps its contrast there as well; the old colour names stay as aliases of the
 * tokens while the screens move to the token names.
 */
class testDesignTokens : public QObject {
    Q_OBJECT

 private slots:
    void defaultPalette_matchesTheDesignSystem_data();
    void defaultPalette_matchesTheDesignSystem();

    void oldNames_followTheirTokens_data();
    void oldNames_followTheirTokens();

    void retiredNames_areGone_data();
    void retiredNames_areGone();

    void contrast_meetsTheDesignSystem_data();
    void contrast_meetsTheDesignSystem();

    void selectionFill_stepsFromNoneToSurfaceSelected_data();
    void selectionFill_stepsFromNoneToSurfaceSelected();
    void selectionFill_defaultsToTheSecondHighestLevel();
    void selectionFill_followsTheLevel();
    void selectionFill_keepsPrimaryTextReadable();

 private:
    static QColor token(const QObject& colors, const char* name) { return colors.property(name).value<QColor>(); }

    // WCAG 2 relative luminance and contrast ratio
    static double luminance(const QColor& color) {
        auto channel = [](double c) { return c <= 0.03928 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4); };
        return 0.2126 * channel(color.redF()) + 0.7152 * channel(color.greenF()) + 0.0722 * channel(color.blueF());
    }

    static double contrast(const QColor& a, const QColor& b) {
        const double la = luminance(a);
        const double lb = luminance(b);
        return (qMax(la, lb) + 0.05) / (qMin(la, lb) + 0.05);
    }
};

void testDesignTokens::defaultPalette_matchesTheDesignSystem_data() {
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("value");

    QTest::newRow("bg") << "bg" << "#000000";
    QTest::newRow("textPrimary") << "textPrimary" << "#d0d0d0";
    QTest::newRow("textSecondary") << "textSecondary" << "#a0a0a0";
    QTest::newRow("textDisabled") << "textDisabled" << "#7a7a7a";
    QTest::newRow("textOnButton") << "textOnButton" << "#ffffff";
    QTest::newRow("surface") << "surface" << "#1e1e1e";
    QTest::newRow("surfaceRaised") << "surfaceRaised" << "#2c2c2c";
    QTest::newRow("surfaceSelected") << "surfaceSelected" << "#595959";
    QTest::newRow("divider") << "divider" << "#3a3a3a";
    QTest::newRow("focusRing") << "focusRing" << "#d0d0d0";
    QTest::newRow("buttonPrimary") << "buttonPrimary" << "#5a5a5a";
    QTest::newRow("redPressed") << "redPressed" << "#ff0d2a";
    QTest::newRow("sliderGradientCool") << "sliderGradientCool" << "#d5f7ff";
    QTest::newRow("sliderGradientWarm") << "sliderGradientWarm" << "#f3aa67";
    QTest::newRow("sliderGradientHeat") << "sliderGradientHeat" << "#c33030";
    QTest::newRow("sliderOnFill") << "sliderOnFill" << "#000000";
    QTest::newRow("sliderGrip") << "sliderGrip" << "#595959";
}

void testDesignTokens::defaultPalette_matchesTheDesignSystem() {
    QFETCH(QString, name);
    QFETCH(QString, value);

    uc::ui::Colors colors;
    QCOMPARE(token(colors, name.toLatin1().constData()).name(), value);
}

void testDesignTokens::oldNames_followTheirTokens_data() {
    QTest::addColumn<QString>("oldName");
    QTest::addColumn<QString>("tokenName");

    QTest::newRow("black") << "black" << "bg";
    QTest::newRow("offwhite") << "offwhite" << "textPrimary";
    QTest::newRow("light") << "light" << "textSecondary";
    QTest::newRow("white") << "white" << "textOnButton";
    QTest::newRow("dark") << "dark" << "surface";
    QTest::newRow("medium") << "medium" << "surfaceRaised";
    QTest::newRow("highlight") << "highlight" << "focusRing";
}

void testDesignTokens::oldNames_followTheirTokens() {
    QFETCH(QString, oldName);
    QFETCH(QString, tokenName);

    uc::ui::Colors colors;
    QCOMPARE(token(colors, oldName.toLatin1().constData()), token(colors, tokenName.toLatin1().constData()));

    // a regenerated palette changes both names together
    colors.generateColorPalette(QColor("#3060a0"));
    QCOMPARE(token(colors, oldName.toLatin1().constData()), token(colors, tokenName.toLatin1().constData()));
}

void testDesignTokens::retiredNames_areGone_data() {
    QTest::addColumn<QString>("name");

    // an old name no screen uses any more is removed rather than kept as an alias
    QTest::newRow("inactiveText") << "inactiveText";
    QTest::newRow("primaryButton") << "primaryButton";
    QTest::newRow("secondaryButton") << "secondaryButton";
}

void testDesignTokens::retiredNames_areGone() {
    QFETCH(QString, name);

    uc::ui::Colors colors;
    QCOMPARE(colors.metaObject()->indexOfProperty(name.toLatin1().constData()), -1);
}

void testDesignTokens::contrast_meetsTheDesignSystem_data() {
    QTest::addColumn<QString>("foreground");
    QTest::addColumn<QString>("background");
    QTest::addColumn<double>("minimum");

    // text on the page background
    QTest::newRow("primary text on black") << "textPrimary" << "bg" << 13.5;
    QTest::newRow("secondary text on black") << "textSecondary" << "bg" << 8.0;
    QTest::newRow("disabled text on black") << "textDisabled" << "bg" << 4.5;
    // text on surfaces
    QTest::newRow("primary text on surface") << "textPrimary" << "surface" << 4.5;
    QTest::newRow("secondary text on surface") << "textSecondary" << "surface" << 4.5;
    QTest::newRow("primary text on raised surface") << "textPrimary" << "surfaceRaised" << 4.5;
    QTest::newRow("secondary text on raised surface") << "textSecondary" << "surfaceRaised" << 4.5;
    // everything on the selection fill is drawn in the primary text colour
    QTest::newRow("primary text on selection fill") << "textPrimary" << "surfaceSelected" << 4.5;
    // pressed elements invert: black content on the primary text colour
    QTest::newRow("pressed content") << "bg" << "textPrimary" << 4.5;
    // button labels; a pressed destructive button label is large text (26 px)
    QTest::newRow("primary button label") << "textOnButton" << "buttonPrimary" << 4.5;
    QTest::newRow("pressed destructive label") << "textOnButton" << "redPressed" << 3.0;
    // the selection ring is a non-text indicator
    QTest::newRow("focus ring on black") << "focusRing" << "bg" << 3.0;
    // slider items: the label (26 px) and the value (30 px) over a fill are large text, the grip a non-text indicator
    QTest::newRow("slider text on the neutral fill") << "sliderOnFill" << "textPrimary" << 4.5;
    QTest::newRow("slider text on the cool end") << "sliderOnFill" << "sliderGradientCool" << 4.5;
    QTest::newRow("slider text on the warm end") << "sliderOnFill" << "sliderGradientWarm" << 4.5;
    QTest::newRow("slider text on the heat end") << "sliderOnFill" << "sliderGradientHeat" << 3.0;
    QTest::newRow("slider grip on the cursor band") << "sliderGrip" << "textPrimary" << 3.0;
}

void testDesignTokens::contrast_meetsTheDesignSystem() {
    QFETCH(QString, foreground);
    QFETCH(QString, background);
    QFETCH(double, minimum);

    uc::ui::Colors colors;
    const double   ratio =
        contrast(token(colors, foreground.toLatin1().constData()), token(colors, background.toLatin1().constData()));
    QVERIFY2(ratio >= minimum, qPrintable(QStringLiteral("%1:1 is below %2:1").arg(ratio, 0, 'f', 2).arg(minimum)));
}

void testDesignTokens::selectionFill_stepsFromNoneToSurfaceSelected_data() {
    QTest::addColumn<int>("level");
    QTest::addColumn<QString>("value");
    QTest::addColumn<int>("alpha");

    // five levels: no fill, then equal steps from black up to surfaceSelected
    QTest::newRow("0: no fill") << 0 << "#000000" << 0;
    QTest::newRow("1") << 1 << "#161616" << 255;
    QTest::newRow("2") << 2 << "#2d2d2d" << 255;
    QTest::newRow("3") << 3 << "#434343" << 255;
    QTest::newRow("4: surfaceSelected") << 4 << "#595959" << 255;
    // a stored level out of range is clamped
    QTest::newRow("below the range") << -1 << "#000000" << 0;
    QTest::newRow("above the range") << 9 << "#595959" << 255;
}

void testDesignTokens::selectionFill_stepsFromNoneToSurfaceSelected() {
    QFETCH(int, level);
    QFETCH(QString, value);
    QFETCH(int, alpha);

    uc::ui::Colors colors;
    const QColor   fill = colors.selectionFillAt(level);
    QCOMPARE(fill.name(), value);
    QCOMPARE(fill.alpha(), alpha);
}

void testDesignTokens::selectionFill_defaultsToTheSecondHighestLevel() {
    uc::ui::Colors colors;
    QCOMPARE(uc::ui::Colors::SELECTION_FILL_DEFAULT_LEVEL, uc::ui::Colors::SELECTION_FILL_MAX_LEVEL - 1);
    QCOMPARE(colors.property("selectionFillMaxLevel").toInt(), uc::ui::Colors::SELECTION_FILL_MAX_LEVEL);
    QCOMPARE(token(colors, "selectionFill"), colors.selectionFillAt(uc::ui::Colors::SELECTION_FILL_DEFAULT_LEVEL));
    // the highest level is the design system's selection fill
    QCOMPARE(colors.selectionFillAt(uc::ui::Colors::SELECTION_FILL_MAX_LEVEL), token(colors, "surfaceSelected"));
}

void testDesignTokens::selectionFill_followsTheLevel() {
    uc::ui::Colors colors;
    QSignalSpy     changed(&colors, &uc::ui::Colors::selectionFillChanged);

    colors.setSelectionFillLevel(0);
    QCOMPARE(changed.count(), 1);
    QCOMPARE(token(colors, "selectionFill").alpha(), 0);

    colors.setSelectionFillLevel(4);
    QCOMPARE(changed.count(), 2);
    QCOMPARE(token(colors, "selectionFill"), token(colors, "surfaceSelected"));

    // the same fill again is no change
    colors.setSelectionFillLevel(4);
    colors.setSelectionFillLevel(9);
    QCOMPARE(changed.count(), 2);
}

void testDesignTokens::selectionFill_keepsPrimaryTextReadable() {
    uc::ui::Colors colors;
    for (int level = 1; level <= uc::ui::Colors::SELECTION_FILL_MAX_LEVEL; ++level) {
        const double ratio = contrast(token(colors, "textPrimary"), colors.selectionFillAt(level));
        QVERIFY2(ratio >= 4.5, qPrintable(QStringLiteral("level %1: %2:1 is below 4.5:1").arg(level).arg(ratio)));
    }
}

QTEST_GUILESS_MAIN(testDesignTokens)

#include "test_design_tokens.moc"
