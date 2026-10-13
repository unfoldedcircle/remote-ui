// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "ui/fonts.h"

/**
 * The type roles of the design system (docs/design-system.md section 4). QML calls them by name, so the test calls
 * them through the meta object as well: a role that is not invokable from QML fails here, not at runtime.
 */
class testTypeRoles : public QObject {
    Q_OBJECT

 private slots:
    void role_matchesTheDesignSystem_data();
    void role_matchesTheDesignSystem();

    void role_isNeverBelowTheFloor_data();
    void role_isNeverBelowTheFloor();

    void display_neverGoesBelowItsMinimum_data();
    void display_neverGoesBelowItsMinimum();

    void proseLineHeight_isReadable();

 private:
    static QFont call(uc::ui::Fonts& fonts, const char* role) {
        QFont      font;
        const bool ok = QMetaObject::invokeMethod(&fonts, role, Q_RETURN_ARG(QFont, font));
        if (!ok) {
            qWarning() << "not invokable:" << role;
        }
        return font;
    }

    static QStringList roles() {
        return {"title", "heading", "label", "menuRow", "prose", "help", "caption", "value", "button", "display"};
    }
};

void testTypeRoles::role_matchesTheDesignSystem_data() {
    QTest::addColumn<QString>("role");
    QTest::addColumn<QString>("family");
    QTest::addColumn<int>("pixelSize");
    QTest::addColumn<QString>("style");

    QTest::newRow("title") << "title" << "Poppins" << 28 << "Medium";
    QTest::newRow("heading") << "heading" << "Poppins" << 26 << "Medium";
    QTest::newRow("label") << "label" << "Poppins" << 30 << "Normal";
    QTest::newRow("menuRow") << "menuRow" << "Poppins" << 28 << "Normal";
    QTest::newRow("prose") << "prose" << "Poppins" << 26 << "Normal";
    QTest::newRow("help") << "help" << "Poppins" << 26 << "Normal";
    QTest::newRow("caption") << "caption" << "Poppins" << 22 << "Normal";
    QTest::newRow("value") << "value" << "Space Mono" << 26 << "Normal";
    QTest::newRow("button") << "button" << "Poppins" << 26 << "Medium";
    QTest::newRow("display") << "display" << "Poppins" << 90 << "Light";
}

void testTypeRoles::role_matchesTheDesignSystem() {
    QFETCH(QString, role);
    QFETCH(QString, family);
    QFETCH(int, pixelSize);
    QFETCH(QString, style);

    uc::ui::Fonts fonts;
    const QFont   font = call(fonts, role.toUtf8().constData());

    QCOMPARE(font.family(), family);
    QCOMPARE(font.pixelSize(), pixelSize);
    QCOMPARE(font.styleName(), style);
}

void testTypeRoles::role_isNeverBelowTheFloor_data() {
    QTest::addColumn<QString>("role");
    for (const QString& role : roles()) {
        QTest::newRow(role.toUtf8().constData()) << role;
    }
}

void testTypeRoles::role_isNeverBelowTheFloor() {
    QFETCH(QString, role);

    uc::ui::Fonts fonts;
    const QFont   font = call(fonts, role.toUtf8().constData());

    QVERIFY2(font.pixelSize() >= uc::ui::Fonts::minimumPixelSize,
             qPrintable(QString("%1 is %2 px").arg(role).arg(font.pixelSize())));
    QVERIFY2(font.styleName() != "Thin", qPrintable(role));
    if (font.styleName() == "Light") {
        QVERIFY2(font.pixelSize() >= uc::ui::Fonts::minimumDisplaySize,
                 qPrintable(QString("%1 is light at %2 px").arg(role).arg(font.pixelSize())));
    }
}

void testTypeRoles::display_neverGoesBelowItsMinimum_data() {
    QTest::addColumn<int>("requested");
    QTest::addColumn<int>("expected");

    QTest::newRow("smaller than the minimum") << 30 << 56;
    QTest::newRow("the minimum") << 56 << 56;
    QTest::newRow("volume") << 90 << 90;
    QTest::newRow("entity state") << 180 << 180;
}

void testTypeRoles::display_neverGoesBelowItsMinimum() {
    QFETCH(int, requested);
    QFETCH(int, expected);

    uc::ui::Fonts fonts;
    QFont         font;
    QVERIFY(QMetaObject::invokeMethod(&fonts, "display", Q_RETURN_ARG(QFont, font), Q_ARG(int, requested)));

    QCOMPARE(font.pixelSize(), expected);
    QCOMPARE(font.styleName(), QString("Light"));
}

void testTypeRoles::proseLineHeight_isReadable() {
    uc::ui::Fonts fonts;
    const qreal   lineHeight = fonts.property("proseLineHeight").toReal();

    QVERIFY(lineHeight >= 1.2);
    QCOMPARE(lineHeight, 1.3);
}

QTEST_MAIN(testTypeRoles)
#include "test_type_roles.moc"
