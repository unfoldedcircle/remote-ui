// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "ui/resources.h"

/**
 * The legal pages follow the links of their documents by asking for the content of the linked file. The base
 * directory arrives as the base URL of the QML text item, "file:" + directory + "/", and the link is whatever the
 * document contains: only a document of the legal directory may be returned.
 */
class testResources : public QObject {
    Q_OBJECT

 private slots:
    void initTestCase();

    void linkContent_relativeLink_isRead();
    void linkContent_subDirectory_reportsItsDirectory();
    void linkContent_doesNotDependOnWorkingDirectory();
    void linkContent_externalUrl_isNotFollowed_data();
    void linkContent_externalUrl_isNotFollowed();
    void linkContent_outsideLegalDirectory_isNotFollowed_data();
    void linkContent_outsideLegalDirectory_isNotFollowed();

 private:
    static void writeFile(const QString& path, const QByteArray& content);
    QString     baseUrl(const QString& subDir) const;

    QTemporaryDir m_root;
    QString       m_legalPath;
};

void testResources::writeFile(const QString& path, const QByteArray& content) {
    QVERIFY(QDir().mkpath(QFileInfo(path).absolutePath()));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(content);
}

QString testResources::baseUrl(const QString& subDir) const {
    // what AboutPage.qml and LicensePage.qml build from the directory reported by aboutInfo
    return "file:" + m_legalPath + "/" + subDir + "/";
}

void testResources::initTestCase() {
    QVERIFY(m_root.isValid());
    m_legalPath = m_root.path() + "/legal";

    writeFile(m_legalPath + "/licenses/README.md", "overview");
    writeFile(m_legalPath + "/licenses/qt.md", "qt license");
    writeFile(m_legalPath + "/licenses/texts/gpl.md", "gpl text");
    writeFile(m_root.path() + "/secret.txt", "not a legal document");
    writeFile(m_root.path() + "/legal-other/other.md", "sibling directory");
}

void testResources::linkContent_relativeLink_isRead() {
    uc::ui::Resources resources(QString(), m_legalPath);
    QSignalSpy        aboutInfo(&resources, &uc::ui::Resources::aboutInfo);

    resources.getLinkContent(baseUrl("licenses"), QStringLiteral("qt.md"));

    QCOMPARE(aboutInfo.count(), 1);
    QCOMPARE(aboutInfo.at(0).at(0).toString(), QStringLiteral("qt license"));
    QCOMPARE(aboutInfo.at(0).at(1).toString(), m_legalPath + "/licenses");
}

void testResources::linkContent_subDirectory_reportsItsDirectory() {
    uc::ui::Resources resources(QString(), m_legalPath);
    QSignalSpy        aboutInfo(&resources, &uc::ui::Resources::aboutInfo);

    resources.getLinkContent(baseUrl("licenses"), QStringLiteral("texts/gpl.md"));

    QCOMPARE(aboutInfo.count(), 1);
    QCOMPARE(aboutInfo.at(0).at(0).toString(), QStringLiteral("gpl text"));
    // images and links of the document are relative to its own directory
    QCOMPARE(aboutInfo.at(0).at(1).toString(), m_legalPath + "/licenses/texts");
}

void testResources::linkContent_doesNotDependOnWorkingDirectory() {
    const QString previous = QDir::currentPath();
    QVERIFY(QDir::setCurrent(m_root.path() + "/legal-other"));

    uc::ui::Resources resources(QString(), m_legalPath);
    QSignalSpy        aboutInfo(&resources, &uc::ui::Resources::aboutInfo);

    resources.getLinkContent(baseUrl("licenses"), QStringLiteral("qt.md"));

    QVERIFY(QDir::setCurrent(previous));
    QCOMPARE(aboutInfo.count(), 1);
    QCOMPARE(aboutInfo.at(0).at(0).toString(), QStringLiteral("qt license"));
}

void testResources::linkContent_externalUrl_isNotFollowed_data() {
    QTest::addColumn<QString>("link");

    QTest::newRow("http") << "http://example.com/qt.md";
    QTest::newRow("https") << "https://example.com/qt.md";
    QTest::newRow("upper case scheme") << "HTTPS://example.com/qt.md";
    QTest::newRow("ftp") << "ftp://example.com/qt.md";
    QTest::newRow("mailto") << "mailto:hello@example.com";
    QTest::newRow("file url") << "file:///etc/passwd";
    QTest::newRow("protocol relative") << "//example.com/qt.md";
    QTest::newRow("empty") << "";
}

void testResources::linkContent_externalUrl_isNotFollowed() {
    QFETCH(QString, link);

    uc::ui::Resources resources(QString(), m_legalPath);
    QSignalSpy        aboutInfo(&resources, &uc::ui::Resources::aboutInfo);

    resources.getLinkContent(baseUrl("licenses"), link);

    // the page is told that there is nothing to show, as for a file that cannot be opened
    QCOMPARE(aboutInfo.count(), 1);
    QVERIFY(aboutInfo.at(0).at(0).toString().isEmpty());
}

void testResources::linkContent_outsideLegalDirectory_isNotFollowed_data() {
    QTest::addColumn<QString>("link");

    QTest::newRow("parent") << "../../secret.txt";
    QTest::newRow("absolute") << m_root.path() + "/secret.txt";
    QTest::newRow("sibling with the same prefix") << "../../legal-other/other.md";
    QTest::newRow("through a sub directory") << "texts/../../../secret.txt";
}

void testResources::linkContent_outsideLegalDirectory_isNotFollowed() {
    QFETCH(QString, link);

    uc::ui::Resources resources(QString(), m_legalPath);
    QSignalSpy        aboutInfo(&resources, &uc::ui::Resources::aboutInfo);

    resources.getLinkContent(baseUrl("licenses"), link);

    QCOMPARE(aboutInfo.count(), 1);
    QVERIFY(aboutInfo.at(0).at(0).toString().isEmpty());
}

QTEST_GUILESS_MAIN(testResources)

#include "test_resources.moc"
