// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QBuffer>
#include <QImageReader>
#include <QtTest>

#include "ui/imageDecoder.h"

using uc::ui::ImageDecoder;

/**
 * Media artwork, driver-provided images and custom icons are decoded from data the UI does not
 * control. The decoder bounds what a decode can cost on the remote: a bitmap is downscaled while
 * decoding, an SVG is rasterized to fit the box instead of at the size it declares, and data the
 * SVG plugin would inflate or parse without a limit is rejected.
 */
class testImageDecoder : public QObject {
    Q_OBJECT

 private slots:
    void initTestCase();

    void smallBitmapIsReturnedAtItsSize();
    void largePngIsDownscaledWhileDecoding();
    void largeJpegIsDownscaledWhileDecoding();
    void svgIsRasterizedToFitTheBox();
    void svgWithHugeDeclaredSizeIsBounded();
    void svgWithOnlyAViewBoxIsRasterized();
    void compressedDataIsRejected();
    void oversizedSvgIsRejected();
    void garbageIsRejected();
    void emptyDataIsRejected();
    void svgFileChecks();
    void svgFileNames();
    void fitSize_data();
    void fitSize();
    void fitSizeOfNothingIsInvalid();

 private:
    bool m_svgSupported = false;

    static QByteArray bitmap(int width, int height, const char* format);
    static QByteArray svg(const QString& sizeAttributes);
    void              requireSvg();
};

void testImageDecoder::initTestCase() {
    m_svgSupported = QImageReader::supportedImageFormats().contains("svg");
}

void testImageDecoder::requireSvg() {
    if (!m_svgSupported) {
        QSKIP("the Qt SVG image format plugin is not available in this Qt installation");
    }
}

QByteArray testImageDecoder::bitmap(int width, int height, const char* format) {
    QImage image(width, height, QImage::Format_RGB32);
    image.fill(Qt::darkCyan);

    QByteArray data;
    QBuffer    buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, format);

    return data;
}

QByteArray testImageDecoder::svg(const QString& sizeAttributes) {
    return QString(
               "<svg xmlns=\"http://www.w3.org/2000/svg\" %1><rect width=\"100%\" height=\"100%\" fill=\"red\"/></svg>")
        .arg(sizeAttributes)
        .toUtf8();
}

void testImageDecoder::smallBitmapIsReturnedAtItsSize() {
    QString      error;
    const QImage image = ImageDecoder::decode(bitmap(100, 50, "PNG"), 1024, &error);

    QVERIFY2(!image.isNull(), qPrintable(error));
    QCOMPARE(image.size(), QSize(100, 50));
}

void testImageDecoder::largePngIsDownscaledWhileDecoding() {
    QString      error;
    const QImage image = ImageDecoder::decode(bitmap(3000, 1500, "PNG"), 1024, &error);

    QVERIFY2(!image.isNull(), qPrintable(error));
    QCOMPARE(image.size(), QSize(1024, 512));
}

void testImageDecoder::largeJpegIsDownscaledWhileDecoding() {
    QString      error;
    const QImage image = ImageDecoder::decode(bitmap(2048, 2048, "JPEG"), 1024, &error);

    QVERIFY2(!image.isNull(), qPrintable(error));
    QCOMPARE(image.size(), QSize(1024, 1024));
}

void testImageDecoder::svgIsRasterizedToFitTheBox() {
    requireSvg();

    QString      error;
    const QImage image = ImageDecoder::decode(svg("width=\"200\" height=\"100\""), 1024, &error);

    QVERIFY2(!image.isNull(), qPrintable(error));
    // fitted into the box with its aspect ratio kept, not drawn at 200 x 100
    QCOMPARE(image.size(), QSize(1024, 512));
}

void testImageDecoder::svgWithHugeDeclaredSizeIsBounded() {
    requireSvg();

    QString      error;
    const QImage image = ImageDecoder::decode(svg("width=\"20000\" height=\"20000\""), 512, &error);

    QVERIFY2(!image.isNull(), qPrintable(error));
    QCOMPARE(image.size(), QSize(512, 512));
}

void testImageDecoder::svgWithOnlyAViewBoxIsRasterized() {
    requireSvg();

    // Icons exported by design tools often carry a viewBox but no width / height.
    QString      error;
    const QImage image = ImageDecoder::decode(svg("viewBox=\"0 0 24 12\""), 96, &error);

    QVERIFY2(!image.isNull(), qPrintable(error));
    QCOMPARE(image.size(), QSize(96, 48));
}

void testImageDecoder::compressedDataIsRejected() {
    // gzip magic number followed by anything: the SVG plugin would try to inflate it without a limit
    QByteArray data = QByteArray::fromHex("1f8b0800");
    data.append(svg("width=\"10\" height=\"10\""));

    QString      error;
    const QImage image = ImageDecoder::decode(data, 1024, &error);

    QVERIFY(image.isNull());
    QVERIFY2(error.contains("compressed"), qPrintable(error));
    QVERIFY(ImageDecoder::isGzip(data));
}

void testImageDecoder::oversizedSvgIsRejected() {
    requireSvg();

    QByteArray data = svg("width=\"10\" height=\"10\"");
    data.insert(data.size() - 6, "<!--" + QByteArray(ImageDecoder::MAX_SVG_BYTES, 'x') + "-->");
    QVERIFY(data.size() > ImageDecoder::MAX_SVG_BYTES);

    QString      error;
    const QImage image = ImageDecoder::decode(data, 1024, &error);

    QVERIFY(image.isNull());
    QVERIFY2(error.contains("larger"), qPrintable(error));
}

void testImageDecoder::garbageIsRejected() {
    QString      error;
    const QImage image = ImageDecoder::decode("this is not an image", 1024, &error);

    QVERIFY(image.isNull());
    QVERIFY(!error.isEmpty());
}

void testImageDecoder::emptyDataIsRejected() {
    QString      error;
    const QImage image = ImageDecoder::decode(QByteArray(), 1024, &error);

    QVERIFY(image.isNull());
    QVERIFY(!error.isEmpty());
}

void testImageDecoder::svgFileChecks() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    auto write = [&dir](const QString& name, const QByteArray& content) {
        QFile file(dir.filePath(name));
        if (!file.open(QIODevice::WriteOnly)) {
            return QString();
        }
        file.write(content);
        return file.fileName();
    };

    const QString good = write("good.svg", svg("width=\"10\" height=\"10\""));
    const QString packed = write("packed.svg", QByteArray::fromHex("1f8b0800") + svg("width=\"10\" height=\"10\""));
    const QString huge = write("huge.svg", QByteArray(ImageDecoder::MAX_SVG_BYTES + 1, ' '));
    const QString exact = write("exact.svg", QByteArray(ImageDecoder::MAX_SVG_BYTES, ' '));

    QString error;
    QVERIFY2(ImageDecoder::checkSvgFile(good, &error), qPrintable(error));
    QVERIFY(ImageDecoder::checkSvgFile(exact, &error));

    QVERIFY(!ImageDecoder::checkSvgFile(packed, &error));
    QVERIFY2(error.contains("compressed"), qPrintable(error));

    QVERIFY(!ImageDecoder::checkSvgFile(huge, &error));
    QVERIFY2(error.contains("larger"), qPrintable(error));

    QVERIFY(!ImageDecoder::checkSvgFile(dir.filePath("missing.svg"), &error));
    QVERIFY2(error.contains("not found"), qPrintable(error));

    // the check only reads the head of the file: an unreadable body is left to the image loader
    QVERIFY(ImageDecoder::checkSvgFile(write("garbage.svg", "<not really svg"), &error));
}

void testImageDecoder::svgFileNames() {
    QVERIFY(ImageDecoder::isSvgFileName("icon.svg"));
    QVERIFY(ImageDecoder::isSvgFileName("ICON.SVG"));
    QVERIFY(ImageDecoder::isSvgFileName("icon.svgz"));
    QVERIFY(!ImageDecoder::isSvgFileName("icon.png"));
    QVERIFY(!ImageDecoder::isSvgFileName("svg"));
    QVERIFY(!ImageDecoder::isSvgFileName("icon.svg.png"));
}

void testImageDecoder::fitSize_data() {
    QTest::addColumn<QSizeF>("source");
    QTest::addColumn<QSizeF>("box");
    QTest::addColumn<QSize>("fitted");

    // the aspect ratios the core accepts for an SVG icon, in the button boxes of the default 4 x 6 grid
    QTest::newRow("1:1 in 120 x 120") << QSizeF(100, 100) << QSizeF(120, 120) << QSize(120, 120);
    QTest::newRow("1:1 in 240 x 120") << QSizeF(100, 100) << QSizeF(240, 120) << QSize(120, 120);
    QTest::newRow("1:1 in 480 x 120") << QSizeF(100, 100) << QSizeF(480, 120) << QSize(120, 120);
    QTest::newRow("1:1 in 480 x 240") << QSizeF(100, 100) << QSizeF(480, 240) << QSize(240, 240);
    QTest::newRow("1:1 in 120 x 480") << QSizeF(100, 100) << QSizeF(120, 480) << QSize(120, 120);
    QTest::newRow("1:1 in 480 x 720") << QSizeF(100, 100) << QSizeF(480, 720) << QSize(480, 480);
    QTest::newRow("2:1 in 120 x 120") << QSizeF(200, 100) << QSizeF(120, 120) << QSize(120, 60);
    QTest::newRow("2:1 in 240 x 120") << QSizeF(200, 100) << QSizeF(240, 120) << QSize(240, 120);
    QTest::newRow("2:1 in 480 x 120") << QSizeF(200, 100) << QSizeF(480, 120) << QSize(240, 120);
    QTest::newRow("2:1 in 480 x 240") << QSizeF(200, 100) << QSizeF(480, 240) << QSize(480, 240);
    QTest::newRow("2:1 in 120 x 480") << QSizeF(200, 100) << QSizeF(120, 480) << QSize(120, 60);
    QTest::newRow("2:1 in 480 x 720") << QSizeF(200, 100) << QSizeF(480, 720) << QSize(480, 240);
    QTest::newRow("3:1 in 120 x 120") << QSizeF(300, 100) << QSizeF(120, 120) << QSize(120, 40);
    QTest::newRow("3:1 in 240 x 120") << QSizeF(300, 100) << QSizeF(240, 120) << QSize(240, 80);
    QTest::newRow("3:1 in 480 x 120") << QSizeF(300, 100) << QSizeF(480, 120) << QSize(360, 120);
    QTest::newRow("3:1 in 480 x 240") << QSizeF(300, 100) << QSizeF(480, 240) << QSize(480, 160);
    QTest::newRow("3:1 in 120 x 480") << QSizeF(300, 100) << QSizeF(120, 480) << QSize(120, 40);
    QTest::newRow("3:1 in 480 x 720") << QSizeF(300, 100) << QSizeF(480, 720) << QSize(480, 160);
    QTest::newRow("4:1 in 120 x 120") << QSizeF(400, 100) << QSizeF(120, 120) << QSize(120, 30);
    QTest::newRow("4:1 in 240 x 120") << QSizeF(400, 100) << QSizeF(240, 120) << QSize(240, 60);
    QTest::newRow("4:1 in 480 x 120") << QSizeF(400, 100) << QSizeF(480, 120) << QSize(480, 120);
    QTest::newRow("4:1 in 480 x 240") << QSizeF(400, 100) << QSizeF(480, 240) << QSize(480, 120);
    QTest::newRow("4:1 in 120 x 480") << QSizeF(400, 100) << QSizeF(120, 480) << QSize(120, 30);
    QTest::newRow("4:1 in 480 x 720") << QSizeF(400, 100) << QSizeF(480, 720) << QSize(480, 120);
    QTest::newRow("1:2 in 120 x 120") << QSizeF(100, 200) << QSizeF(120, 120) << QSize(60, 120);
    QTest::newRow("1:2 in 240 x 120") << QSizeF(100, 200) << QSizeF(240, 120) << QSize(60, 120);
    QTest::newRow("1:2 in 480 x 120") << QSizeF(100, 200) << QSizeF(480, 120) << QSize(60, 120);
    QTest::newRow("1:2 in 480 x 240") << QSizeF(100, 200) << QSizeF(480, 240) << QSize(120, 240);
    QTest::newRow("1:2 in 120 x 480") << QSizeF(100, 200) << QSizeF(120, 480) << QSize(120, 240);
    QTest::newRow("1:2 in 480 x 720") << QSizeF(100, 200) << QSizeF(480, 720) << QSize(360, 720);
    QTest::newRow("1:4 in 120 x 120") << QSizeF(100, 400) << QSizeF(120, 120) << QSize(30, 120);
    QTest::newRow("1:4 in 240 x 120") << QSizeF(100, 400) << QSizeF(240, 120) << QSize(30, 120);
    QTest::newRow("1:4 in 480 x 120") << QSizeF(100, 400) << QSizeF(480, 120) << QSize(30, 120);
    QTest::newRow("1:4 in 480 x 240") << QSizeF(100, 400) << QSizeF(480, 240) << QSize(60, 240);
    QTest::newRow("1:4 in 120 x 480") << QSizeF(100, 400) << QSizeF(120, 480) << QSize(120, 480);
    QTest::newRow("1:4 in 480 x 720") << QSizeF(100, 400) << QSizeF(480, 720) << QSize(180, 720);

    // an icon in a square box elsewhere in the UI, such as an entity tile
    QTest::newRow("4:1 in 100 x 100") << QSizeF(400, 100) << QSizeF(100, 100) << QSize(100, 25);
    // a Remote Two cell is not square
    QTest::newRow("1:1 in 120 x 129") << QSizeF(24, 24) << QSizeF(120, 129) << QSize(120, 120);
    QTest::newRow("viewBox 70 x 35 in 240 x 120") << QSizeF(70, 35) << QSizeF(240, 120) << QSize(240, 120);
    // rounded to whole pixels, but never below one pixel per side
    QTest::newRow("3:1 in 100 x 100") << QSizeF(300, 100) << QSizeF(100, 100) << QSize(100, 33);
    QTest::newRow("1000:1 in 100 x 100") << QSizeF(1000, 1) << QSizeF(100, 100) << QSize(100, 1);
}

void testImageDecoder::fitSize() {
    QFETCH(QSizeF, source);
    QFETCH(QSizeF, box);
    QFETCH(QSize, fitted);

    QCOMPARE(ImageDecoder::fitSize(source, box), fitted);
}

void testImageDecoder::fitSizeOfNothingIsInvalid() {
    QVERIFY(!ImageDecoder::fitSize(QSizeF(), QSizeF(120, 120)).isValid());
    QVERIFY(!ImageDecoder::fitSize(QSizeF(0, 100), QSizeF(120, 120)).isValid());
    QVERIFY(!ImageDecoder::fitSize(QSizeF(200, 100), QSizeF()).isValid());
    QVERIFY(!ImageDecoder::fitSize(QSizeF(200, 100), QSizeF(0, 0)).isValid());
    QVERIFY(!ImageDecoder::fitSize(QSizeF(200, 100), QSizeF(240, 0)).isValid());
}

QTEST_MAIN(testImageDecoder)

#include "test_image_decoder.moc"
