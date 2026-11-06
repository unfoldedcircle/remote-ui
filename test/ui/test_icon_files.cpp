// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "ui/imageDecoder.h"
#include "ui/resources.h"

/**
 * Custom icons and TV channel icons are files in the resource directory, named by the core as
 * `custom:<file>` and `ctv:<file>`. Any image format the image loader decodes is accepted, SVG
 * included. An SVG the checks reject - compressed, or above the size limit - is not offered in
 * the icon selector and resolves to the placeholder glyph instead of a file the loader would
 * choke on.
 */
class testIconFiles : public QObject {
    Q_OBJECT

 private slots:
    void initTestCase();

    void customIconListOffersBitmapsAndSvg();
    void customIconListIgnoresOtherFiles();
    void customSvgIconResolvesToItsFile();
    void bitmapIconsStillResolve();
    void tvChannelSvgIconResolves();
    void missingIconIsEmpty();
    void compressedSvgIconShowsThePlaceholder();
    void oversizedSvgIconShowsThePlaceholder();
    void placeholderIsAGlyph();
    void iconWithoutPrefixIsEmpty();

 private:
    QTemporaryDir m_dir;

    uc::ui::Resources* createResources() { return new uc::ui::Resources(m_dir.path(), QString(), this); }
    QString            iconPath(const QString& name) const { return m_dir.path() + "/Icon/" + name; }
    static QByteArray  svg() {
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"10\" height=\"10\">"
                "<rect width=\"10\" height=\"10\"/></svg>";
    }
    static void write(const QString& path, const QByteArray& content);
};

void testIconFiles::write(const QString& path, const QByteArray& content) {
    QFile file(path);
    QVERIFY2(file.open(QIODevice::WriteOnly), qPrintable(path));
    file.write(content);
}

void testIconFiles::initTestCase() {
    QVERIFY(m_dir.isValid());
    QVERIFY(QDir(m_dir.path()).mkpath("Icon"));
    QVERIFY(QDir(m_dir.path()).mkpath("TvChannelIcon"));

    QImage png(10, 10, QImage::Format_RGB32);
    png.fill(Qt::red);
    QVERIFY(png.save(iconPath("sofa.png")));
    QVERIFY(png.save(iconPath("Photo.JPG"), "JPEG"));

    write(iconPath("tv.svg"), svg());
    write(iconPath("packed.svg"), QByteArray::fromHex("1f8b0800") + svg());
    write(iconPath("huge.svg"), QByteArray(uc::ui::ImageDecoder::MAX_SVG_BYTES + 1, ' '));
    write(iconPath("notes.txt"), "not an icon");
    write(m_dir.path() + "/TvChannelIcon/bbc1.svg", svg());
}

void testIconFiles::customIconListOffersBitmapsAndSvg() {
    QScopedPointer<uc::ui::Resources> resources(createResources());

    const QStringList list = resources->getCustomIconList();

    QVERIFY(list.contains("custom:sofa.png"));
    QVERIFY(list.contains("custom:Photo.JPG"));
    QVERIFY(list.contains("custom:tv.svg"));
}

void testIconFiles::customIconListIgnoresOtherFiles() {
    QScopedPointer<uc::ui::Resources> resources(createResources());

    const QStringList list = resources->getCustomIconList();

    // rejected SVG files would only show as the placeholder: not offered
    QVERIFY(!list.contains("custom:packed.svg"));
    QVERIFY(!list.contains("custom:huge.svg"));
    QVERIFY(!list.contains("custom:notes.txt"));
    QCOMPARE(list.size(), 3);
}

void testIconFiles::customSvgIconResolvesToItsFile() {
    QScopedPointer<uc::ui::Resources> resources(createResources());

    QCOMPARE(resources->getIcon("custom:tv.svg"), "file:" + iconPath("tv.svg"));
}

void testIconFiles::bitmapIconsStillResolve() {
    QScopedPointer<uc::ui::Resources> resources(createResources());

    QCOMPARE(resources->getIcon("custom:sofa.png"), "file:" + iconPath("sofa.png"));
    QCOMPARE(resources->getIcon("custom:Photo.JPG"), "file:" + iconPath("Photo.JPG"));
}

void testIconFiles::tvChannelSvgIconResolves() {
    QScopedPointer<uc::ui::Resources> resources(createResources());

    QCOMPARE(resources->getIcon("ctv:bbc1.svg"), "file:" + m_dir.path() + "/TvChannelIcon/bbc1.svg");
}

void testIconFiles::missingIconIsEmpty() {
    QScopedPointer<uc::ui::Resources> resources(createResources());

    // a file that is not there leaves the icon area empty, as before
    QCOMPARE(resources->getIcon("custom:missing.svg"), QString());
    QCOMPARE(resources->getIcon("custom:missing.png"), QString());
    QCOMPARE(resources->getIcon("ctv:missing.svg"), QString());
}

void testIconFiles::compressedSvgIconShowsThePlaceholder() {
    QScopedPointer<uc::ui::Resources> resources(createResources());

    const QString icon = resources->getIcon("custom:packed.svg");

    QVERIFY(!icon.isEmpty());
    QCOMPARE(icon, resources->getIconPlaceholder());
}

void testIconFiles::oversizedSvgIconShowsThePlaceholder() {
    QScopedPointer<uc::ui::Resources> resources(createResources());

    QCOMPARE(resources->getIcon("custom:huge.svg"), resources->getIconPlaceholder());
}

void testIconFiles::placeholderIsAGlyph() {
    QScopedPointer<uc::ui::Resources> resources(createResources());

    const QString placeholder = resources->getIconPlaceholder();

    QVERIFY(!placeholder.isEmpty());
    QVERIFY(!placeholder.startsWith("file:"));
    QCOMPARE(placeholder, resources->getIcon("uc:circle-question"));
}

void testIconFiles::iconWithoutPrefixIsEmpty() {
    QScopedPointer<uc::ui::Resources> resources(createResources());

    QCOMPARE(resources->getIcon("tv.svg"), QString());
}

QTEST_MAIN(testIconFiles)

#include "test_icon_files.moc"
