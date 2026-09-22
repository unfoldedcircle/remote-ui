// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QFontDatabase>
#include <QtTest>

#include "ui/resources.h"

/**
 * The embedded icon font comes in two editions: the Free one in the repository and the licensed
 * Pro one, overlaid by the firmware build (docs/icon-font.md). The Free edition does not have
 * every icon, so an icon it cannot draw has to be replaced by its mapped alternative or by the
 * placeholder - and the icon selection must not offer icons that would end up as the placeholder.
 *
 * The tests run against whichever edition is embedded: with the Pro font every name resolves on
 * its own and the fallback cases are skipped.
 */
class testIconFont : public QObject {
    Q_OBJECT

 private slots:
    void initTestCase();

    void fontLoadsWithItsOwnFamily();
    void mappedIconIsReturnedUnchanged();
    void unknownIconReturnsEmpty();
    void iconMissingFromTheFontUsesItsFallback();
    void iconMissingFromTheFontWithoutFallbackUsesThePlaceholder();
    void iconListOnlyOffersIconsTheFontCanDraw();
    void withoutAFontEveryMappedIconIsReturned();

 private:
    QString m_family;

    // An icon every edition has, one that only the Pro edition has and that has a fallback
    // mapping, and one that only the Pro edition has without a fallback mapping.
    const QString m_common = "lightbulb";
    const QString m_fallbackIcon = "keyboard-down";
    const QString m_fallbackTarget = "keyboard";
    const QString m_proOnlyIcon = "abacus";
    const QString m_placeholder = "circle-question";

    uc::ui::Resources* createResources() { return new uc::ui::Resources(QString(), QString(), this); }
    bool               fontHasIcon(const QString& name);
};

void testIconFont::initTestCase() {
    const int id = QFontDatabase::addApplicationFont(":icon-font.ttf");
    QVERIFY2(id != -1, "the icon font resource could not be loaded");

    const QStringList families = QFontDatabase::applicationFontFamilies(id);
    QVERIFY2(!families.isEmpty(), "the icon font has no family name");

    m_family = families.first();
}

bool testIconFont::fontHasIcon(const QString& name) {
    QScopedPointer<uc::ui::Resources> resources(createResources());
    const QString                     glyph = resources->getIcon("uc:" + name);

    resources->setIconFont(m_family);
    return resources->getIcon("uc:" + name) == glyph;
}

void testIconFont::fontLoadsWithItsOwnFamily() {
    // The family is read from the font instead of being hard coded, because it differs between
    // the editions. Whatever it is, it must not be empty and not carry the Reserved Font Name.
    QVERIFY(!m_family.isEmpty());
    QVERIFY(!m_family.contains("Font Awesome"));
}

void testIconFont::mappedIconIsReturnedUnchanged() {
    QScopedPointer<uc::ui::Resources> resources(createResources());
    resources->setIconFont(m_family);

    const QString glyph = resources->getIcon("uc:" + m_common);

    QVERIFY(!glyph.isEmpty());
    QCOMPARE(glyph, QString(QChar(0xf0eb)));
}

void testIconFont::unknownIconReturnsEmpty() {
    QScopedPointer<uc::ui::Resources> resources(createResources());
    resources->setIconFont(m_family);

    QCOMPARE(resources->getIcon("uc:no-such-icon-here"), QString());
}

void testIconFont::iconMissingFromTheFontUsesItsFallback() {
    if (fontHasIcon(m_fallbackIcon)) {
        QSKIP("the embedded font has this icon: the Pro edition needs no fallback");
    }

    QScopedPointer<uc::ui::Resources> resources(createResources());
    resources->setIconFont(m_family);

    QCOMPARE(resources->getIcon("uc:" + m_fallbackIcon), resources->getIcon("uc:" + m_fallbackTarget));
}

void testIconFont::iconMissingFromTheFontWithoutFallbackUsesThePlaceholder() {
    if (fontHasIcon(m_proOnlyIcon)) {
        QSKIP("the embedded font has this icon: the Pro edition needs no placeholder");
    }

    QScopedPointer<uc::ui::Resources> resources(createResources());
    resources->setIconFont(m_family);

    QCOMPARE(resources->getIcon("uc:" + m_proOnlyIcon), resources->getIcon("uc:" + m_placeholder));
}

void testIconFont::iconListOnlyOffersIconsTheFontCanDraw() {
    QScopedPointer<uc::ui::Resources> resources(createResources());
    resources->setIconFont(m_family);

    const QStringList list = resources->getIconList();

    QVERIFY(!list.isEmpty());
    QVERIFY(list.contains("uc:" + m_common));

    if (!fontHasIcon(m_proOnlyIcon)) {
        QVERIFY(!list.contains("uc:" + m_proOnlyIcon));
    }
}

void testIconFont::withoutAFontEveryMappedIconIsReturned() {
    // No font set: the icons are rendered by whatever QML picks, so nothing is replaced.
    QScopedPointer<uc::ui::Resources> resources(createResources());

    QVERIFY(!resources->getIcon("uc:" + m_proOnlyIcon).isEmpty());
    QVERIFY(resources->getIconList().contains("uc:" + m_proOnlyIcon));
}

QTEST_MAIN(testIconFont)

#include "test_icon_font.moc"
