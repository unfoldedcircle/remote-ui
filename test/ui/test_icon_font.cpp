// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QFile>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRawFont>
#include <QTemporaryDir>
#include <QtTest>

#include "ui/iconFont.h"
#include "ui/resources.h"

/**
 * The icon font comes in two editions: the Free one embedded in the binary and the licensed Pro
 * one, which the firmware installs as a file and names in UC_ICON_FONT_PATH (docs/icon-font.md).
 * The Free edition does not have every icon, so an icon it cannot draw has to be replaced by its
 * mapped alternative or by the placeholder - and the icon selection must not offer icons that
 * would end up as the placeholder.
 *
 * The tests run against the embedded font, and check that an external font is only used when it
 * names a loadable file.
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
    void iconMissingFromTheFontIsNotTakenFromAnotherFont();
    void iconListOnlyOffersIconsTheFontCanDraw();
    void withoutAFontEveryMappedIconIsReturned();
    void defaultIconOfTheCoreIsDrawable_data();
    void defaultIconOfTheCoreIsDrawable();

    void embeddedFontIsUsedWithoutAnOverride();
    void embeddedFontIsUsedWhenTheOverrideIsMissing();
    void embeddedFontIsUsedWhenTheOverrideIsNotAFont();
    void externalFontIsUsedWhenItLoads();

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

void testIconFont::iconMissingFromTheFontIsNotTakenFromAnotherFont() {
    // Qt draws a code point the icon font lacks with any installed font that has it (OpenSymbol and DejaVu have
    // glyphs at Font Awesome code points). Such an icon is missing all the same: it gets its fallback or the
    // placeholder, never the other font's unrelated glyph. Find one on this system: QFontMetrics answers for the
    // fallback fonts too, the font file only for itself.
    QFile mappingFile(":icon-mapping.json");
    QVERIFY(mappingFile.open(QIODevice::ReadOnly));
    const QJsonObject mapping = QJsonDocument::fromJson(mappingFile.readAll()).object();

    const QRawFont     iconFont(QStringLiteral(":icon-font.ttf"), 16);
    const QFontMetrics withFallbacks{QFont(m_family)};
    QString            name;

    for (auto i = mapping.constBegin(); i != mapping.constEnd() && name.isEmpty(); ++i) {
        const uint codePoint = i.value().toString().toUcs4().value(0);
        if (!iconFont.supportsCharacter(codePoint) && withFallbacks.inFontUcs4(codePoint)) {
            name = i.key();
        }
    }
    if (name.isEmpty()) {
        QSKIP("no font on this system has a glyph at a code point the icon font lacks");
    }

    QScopedPointer<uc::ui::Resources> resources(createResources());
    resources->setIconFont(m_family);

    QVERIFY2(resources->getIcon("uc:" + name) != mapping.value(name).toString(), qPrintable(name));
    QVERIFY2(!resources->getIconList().contains("uc:" + name), qPrintable(name));
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

void testIconFont::defaultIconOfTheCoreIsDrawable_data() {
    QTest::addColumn<QString>("name");

    // The icons remote-core assigns on its own, which the UI shows without naming them in its sources, so
    // `tools/icon-font.py check-mapping` cannot see them. Extend the list when the core adds a default.
    const QList<QPair<const char*, QStringList>> defaults = {
        // the core's default icon per entity type
        {"entity",
         {"activity", "blind", "button", "button-brightness", "climate", "light", "list-dropdown", "macro",
          "mediaplayer", "microphone", "remote", "sensor", "switch"}},
        // media browser thumbnails without an image, which the core sends as icon://uc:...
        {"media",
         {"browser", "compact-disc", "file-music", "film", "folder", "gamepad", "globe-wifi", "image", "list-music",
          "masks-theater", "music", "photo-film", "podcast", "radio", "tv-retro", "user-music", "video"}},
        // the button pages the core generates for IR remotes
        {"ir",
         {"bw", "ff", "next", "pause", "play", "play-pause", "playlist", "prev", "rec", "repeat", "shuffle", "stop"}},
        // the button pages the core generates for Bluetooth peripherals
        {"bt",
         {"back", "bw", "ff", "home", "menu", "next", "pause", "play", "play-pause", "prev", "rec", "stop", "system",
          "tv"}},
        // integration without an icon, IR codeset remote, the default activity group (migrations)
        {"other", {"integration", "remote", "popcorn"}},
    };

    for (const auto& group : defaults) {
        for (const QString& name : group.second) {
            QTest::addRow("%s: %s", group.first, qPrintable(name)) << name;
        }
    }
}

void testIconFont::defaultIconOfTheCoreIsDrawable() {
    QFETCH(QString, name);

    QScopedPointer<uc::ui::Resources> resources(createResources());
    resources->setIconFont(m_family);

    const QString glyph = resources->getIcon("uc:" + name);

    QVERIFY2(!glyph.isEmpty(), "not in the icon mapping");
    QVERIFY2(glyph != resources->getIcon("uc:" + m_placeholder),
             "drawn as the placeholder: add a fallback to resources/icons/icon-fallback.json");
}

void testIconFont::embeddedFontIsUsedWithoutAnOverride() {
    QCOMPARE(uc::ui::IconFont::select(QString()), uc::ui::IconFont::embeddedPath);

    const uc::ui::IconFontResult result = uc::ui::IconFont::load(QString());

    QCOMPARE(result.path, uc::ui::IconFont::embeddedPath);
    QCOMPARE(result.family, m_family);
}

void testIconFont::embeddedFontIsUsedWhenTheOverrideIsMissing() {
    // The firmware names a file that is not there: a warning, and the embedded font.
    const QString missing = QDir::temp().filePath("uc-icon-font-that-does-not-exist.ttf");
    QVERIFY(!QFile::exists(missing));

    QCOMPARE(uc::ui::IconFont::select(missing), uc::ui::IconFont::embeddedPath);

    const uc::ui::IconFontResult result = uc::ui::IconFont::load(missing);

    QCOMPARE(result.path, uc::ui::IconFont::embeddedPath);
    QCOMPARE(result.family, m_family);
}

void testIconFont::embeddedFontIsUsedWhenTheOverrideIsNotAFont() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("not-a-font.ttf");
    QFile         file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("this is not a font");
    file.close();

    // The file is readable, so it is selected - and rejected when it does not load.
    QCOMPARE(uc::ui::IconFont::select(path), path);

    const uc::ui::IconFontResult result = uc::ui::IconFont::load(path);

    QCOMPARE(result.path, uc::ui::IconFont::embeddedPath);
    QCOMPARE(result.family, m_family);
}

void testIconFont::externalFontIsUsedWhenItLoads() {
    // Stand-in for the font the firmware installs: a copy of the embedded one outside the binary.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("icon-font.ttf");
    QVERIFY(QFile::copy(uc::ui::IconFont::embeddedPath, path));

    const uc::ui::IconFontResult result = uc::ui::IconFont::load(path);

    QCOMPARE(result.path, path);
    QVERIFY(!result.family.isEmpty());
    QVERIFY(!result.family.contains("Font Awesome"));
}

QTEST_MAIN(testIconFont)

#include "test_icon_font.moc"
