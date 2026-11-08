// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QImageReader>
#include <QtTest>

#include "ui/imageDecoder.h"
#include "ui/resources.h"

/**
 * Custom icons and TV channel icons are files in the resource directory, named by the core as
 * `custom:<file>` and `ctv:<file>`. Any image format the image loader decodes is accepted, SVG
 * included. An SVG the checks reject - compressed, or above the size limit - is not offered in
 * the icon selector and resolves to the placeholder glyph instead of a file the loader would
 * choke on. An SVG icon is drawn at the largest size with its own aspect ratio that fits its box.
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
    void svgIconSize_data();
    void svgIconSize();
    void tvChannelSvgIconSize();
    void svgIconSizeFollowsAReplacedFile();
    void iconSizeIsInvalidWhenUnknown();

 private:
    QTemporaryDir m_dir;

    uc::ui::Resources* createResources() { return new uc::ui::Resources(m_dir.path(), QString(), this); }
    QString            iconPath(const QString& name) const { return m_dir.path() + "/Icon/" + name; }
    static QByteArray  svg() {
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"10\" height=\"10\">"
                "<rect width=\"10\" height=\"10\"/></svg>";
    }
    static void write(const QString& path, const QByteArray& content);

    // a second resource directory for the size lookup, so the fixtures don't show in the icon list tests
    QTemporaryDir      m_sizeDir;
    uc::ui::Resources* createSizeResources() { return new uc::ui::Resources(m_sizeDir.path(), QString(), this); }
    QString            sizeIconPath(const QString& name) const { return m_sizeDir.path() + "/Icon/" + name; }
    static QByteArray  svg(const QString& rootAttributes) {
        return QString("<svg xmlns=\"http://www.w3.org/2000/svg\" %1><circle cx=\"50\" cy=\"50\" r=\"50\"/></svg>")
            .arg(rootAttributes)
            .toUtf8();
    }
    void requireSvg();
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

    QVERIFY(m_sizeDir.isValid());
    QVERIFY(QDir(m_sizeDir.path()).mkpath("Icon"));
    QVERIFY(QDir(m_sizeDir.path()).mkpath("TvChannelIcon"));

    // the aspect ratios the core accepts for an SVG icon, from a viewBox alone
    write(sizeIconPath("1x1.svg"), svg("viewBox=\"0 0 100 100\""));
    write(sizeIconPath("2x1.svg"), svg("viewBox=\"0 0 200 100\""));
    write(sizeIconPath("3x1.svg"), svg("viewBox=\"0 0 300 100\""));
    write(sizeIconPath("4x1.svg"), svg("viewBox=\"0 0 400 100\""));
    write(sizeIconPath("1x2.svg"), svg("viewBox=\"0 0 100 200\""));
    write(sizeIconPath("1x4.svg"), svg("viewBox=\"0 0 100 400\""));
    // 2:1 with the root width and height in different units
    write(sizeIconPath("2x1-unitless.svg"), svg("viewBox=\"0 0 200 100\" width=\"20\" height=\"10\""));
    write(sizeIconPath("2x1-px.svg"), svg("viewBox=\"0 0 200 100\" width=\"20px\" height=\"10px\""));
    write(sizeIconPath("2x1-mm.svg"), svg("viewBox=\"0 0 200 100\" width=\"20mm\" height=\"10mm\""));
    write(sizeIconPath("2x1-em.svg"), svg("viewBox=\"0 0 200 100\" width=\"1em\" height=\"1em\""));
    write(sizeIconPath("2x1-percent.svg"), svg("viewBox=\"0 0 200 100\" width=\"100%\" height=\"100%\""));
    write(sizeIconPath("2x1-auto.svg"), svg("viewBox=\"0 0 200 100\" width=\"auto\" height=\"auto\""));
    // root width and height that disagree with the viewBox: Qt draws the root size
    write(sizeIconPath("mismatch.svg"), svg("viewBox=\"0 0 200 100\" width=\"100\" height=\"100\""));
    write(m_sizeDir.path() + "/TvChannelIcon/wide.svg", svg("viewBox=\"0 0 400 100\""));

    QVERIFY(png.save(sizeIconPath("sofa.png")));
    write(sizeIconPath("packed.svg"), QByteArray::fromHex("1f8b0800") + svg("viewBox=\"0 0 200 100\""));
    write(sizeIconPath("broken.svg"), "<svg this is not");
}

void testIconFiles::requireSvg() {
    if (!QImageReader::supportedImageFormats().contains("svg")) {
        QSKIP("the Qt SVG image format plugin is not available in this Qt installation");
    }
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

void testIconFiles::svgIconSize_data() {
    QTest::addColumn<QString>("id");
    QTest::addColumn<QSize>("box");
    QTest::addColumn<QSize>("size");

    QTest::newRow("1:1 on a 1 x 1 button") << "custom:1x1.svg" << QSize(120, 120) << QSize(120, 120);
    QTest::newRow("1:1 on a 2 x 1 button") << "custom:1x1.svg" << QSize(240, 120) << QSize(120, 120);
    QTest::newRow("2:1 on a 2 x 1 button") << "custom:2x1.svg" << QSize(240, 120) << QSize(240, 120);
    QTest::newRow("2:1 on a 1 x 1 button") << "custom:2x1.svg" << QSize(120, 120) << QSize(120, 60);
    QTest::newRow("3:1 on a 4 x 1 button") << "custom:3x1.svg" << QSize(480, 120) << QSize(360, 120);
    QTest::newRow("4:1 on a 4 x 1 button") << "custom:4x1.svg" << QSize(480, 120) << QSize(480, 120);
    QTest::newRow("4:1 on a 4 x 2 button") << "custom:4x1.svg" << QSize(480, 240) << QSize(480, 120);
    QTest::newRow("4:1 on a whole page") << "custom:4x1.svg" << QSize(480, 720) << QSize(480, 120);
    QTest::newRow("4:1 in a square box") << "custom:4x1.svg" << QSize(100, 100) << QSize(100, 25);
    QTest::newRow("1:2 on a 2 x 1 button") << "custom:1x2.svg" << QSize(240, 120) << QSize(60, 120);
    QTest::newRow("1:4 on a 1 x 4 button") << "custom:1x4.svg" << QSize(120, 480) << QSize(120, 480);
    QTest::newRow("1:4 on a 4 x 1 button") << "custom:1x4.svg" << QSize(480, 120) << QSize(30, 120);

    QTest::newRow("unitless width and height") << "custom:2x1-unitless.svg" << QSize(240, 240) << QSize(240, 120);
    QTest::newRow("px width and height") << "custom:2x1-px.svg" << QSize(240, 240) << QSize(240, 120);
    QTest::newRow("mm width and height") << "custom:2x1-mm.svg" << QSize(240, 240) << QSize(240, 120);
    QTest::newRow("em width and height") << "custom:2x1-em.svg" << QSize(240, 240) << QSize(240, 120);
    QTest::newRow("% width and height") << "custom:2x1-percent.svg" << QSize(240, 240) << QSize(240, 120);
    QTest::newRow("auto width and height") << "custom:2x1-auto.svg" << QSize(240, 240) << QSize(240, 120);
    QTest::newRow("width and height disagree with the viewBox")
        << "custom:mismatch.svg" << QSize(240, 120) << QSize(120, 120);
}

void testIconFiles::svgIconSize() {
    requireSvg();

    QFETCH(QString, id);
    QFETCH(QSize, box);
    QFETCH(QSize, size);

    QScopedPointer<uc::ui::Resources> resources(createSizeResources());

    QCOMPARE(resources->getIconSize(id, box.width(), box.height()), size);
}

void testIconFiles::tvChannelSvgIconSize() {
    requireSvg();

    QScopedPointer<uc::ui::Resources> resources(createSizeResources());

    QCOMPARE(resources->getIconSize("ctv:wide.svg", 240, 120), QSize(240, 60));
}

void testIconFiles::svgIconSizeFollowsAReplacedFile() {
    requireSvg();

    QScopedPointer<uc::ui::Resources> resources(createSizeResources());
    const QString                     path = sizeIconPath("logo.svg");

    write(path, svg("viewBox=\"0 0 100 100\""));
    QCOMPARE(resources->getIconSize("custom:logo.svg", 240, 120), QSize(120, 120));

    // a file of another size
    write(path, svg("viewBox=\"0 0 2000 1000\""));
    QCOMPARE(resources->getIconSize("custom:logo.svg", 240, 120), QSize(240, 120));

    // a file of the same size, written later
    write(path, svg("viewBox=\"0 0 1000 1000\""));
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadWrite));
    QVERIFY(file.setFileTime(QDateTime::currentDateTime().addSecs(10), QFileDevice::FileModificationTime));
    file.close();
    QCOMPARE(resources->getIconSize("custom:logo.svg", 240, 120), QSize(120, 120));
}

void testIconFiles::iconSizeIsInvalidWhenUnknown() {
    QScopedPointer<uc::ui::Resources> resources(createSizeResources());

    // decoded at the box size, as before
    QVERIFY(!resources->getIconSize("custom:sofa.png", 240, 120).isValid());
    QVERIFY(!resources->getIconSize("uc:power-on", 240, 120).isValid());
    QVERIFY(!resources->getIconSize("custom:missing.svg", 240, 120).isValid());
    QVERIFY(!resources->getIconSize("custom:packed.svg", 240, 120).isValid());
    QVERIFY(!resources->getIconSize("custom:broken.svg", 240, 120).isValid());
    QVERIFY(!resources->getIconSize("2x1.svg", 240, 120).isValid());
    QVERIFY(!resources->getIconSize("", 240, 120).isValid());

    // the box before it has its size
    QVERIFY(!resources->getIconSize("custom:2x1.svg", 0, 0).isValid());
    QVERIFY(!resources->getIconSize("custom:2x1.svg", 240, 0).isValid());
}

QTEST_MAIN(testIconFiles)

#include "test_icon_files.moc"
