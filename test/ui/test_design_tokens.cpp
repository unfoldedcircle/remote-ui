// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>
#include <cmath>

#include "ui/colors.h"

/**
 * The colour tokens of the design system (docs/design-system.md, ADR 0019). The palette is chosen against the raised
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

    void contrast_meetsTheDesignSystem_data();
    void contrast_meetsTheDesignSystem();

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
    QTest::newRow("inactiveText") << "inactiveText" << "textDisabled";
    QTest::newRow("white") << "white" << "textOnButton";
    QTest::newRow("dark") << "dark" << "surface";
    QTest::newRow("medium") << "medium" << "surfaceRaised";
    QTest::newRow("highlight") << "highlight" << "focusRing";
    QTest::newRow("primaryButton") << "primaryButton" << "buttonPrimary";
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

QTEST_GUILESS_MAIN(testDesignTokens)

#include "test_design_tokens.moc"
