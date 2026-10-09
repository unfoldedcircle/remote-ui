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

    void isMarkdownFile_data();
    void isMarkdownFile();
    void licenseBlocks_plainText_oneBlockPerLine();
    void licenseBlocks_overview_splitsAtSecondLevelHeadings();
    void licenseBlocks_linkedDocument_splitsAtEveryHeadingOutsideCode();
    void licenseBlocks_headings_atTextSize();
    void licenseBlocks_webAndMailLinks_areText_data();
    void licenseBlocks_webAndMailLinks_areText();
    void licenseBlocks_documentLinks_areAnchors();
    void licenseAnchorBlock_findsHeading();
    void licenseBlocks_code_isText();
    void licenseBlocks_textTable_keepsItsRows();
    void licenseBlocks_longCode_isSplitAtParagraphs();
    void licenseBlocks_longCodeParagraph_isSplitAtALine();

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

// The Licenses page shows a linked file as Markdown only when its name ends in ".md"; it showed every linked file as
// rich text line by line, so a Markdown license showed its markup
void testResources::isMarkdownFile_data() {
    QTest::addColumn<QString>("link");
    QTest::addColumn<bool>("markdown");

    QTest::newRow("markdown") << "web-configurator_licenses.md" << true;
    QTest::newRow("upper case") << "NOTICE.MD" << true;
    QTest::newRow("with fragment") << "texts/gpl.md#section" << true;
    QTest::newRow("text") << "qt/LICENSE.txt" << false;
    QTest::newRow("no extension") << "LICENSE" << false;
    QTest::newRow("html") << "remote-core_licenses.html" << false;
    QTest::newRow("md inside the name") << "notes.md.txt" << false;
}

void testResources::isMarkdownFile() {
    QFETCH(QString, link);
    QFETCH(bool, markdown);

    QCOMPARE(uc::ui::Resources(QString(), m_legalPath).isMarkdownFile(link), markdown);
}

void testResources::licenseBlocks_plainText_oneBlockPerLine() {
    const QStringList blocks =
        uc::ui::Resources(QString(), m_legalPath).licenseBlocks("# Not a heading\n\nsecond line", false, false);

    QCOMPARE(blocks, QStringList({"# Not a heading", "", "second line"}));
}

void testResources::licenseBlocks_overview_splitsAtSecondLevelHeadings() {
    const QStringList blocks =
        uc::ui::Resources(QString(), m_legalPath)
            .licenseBlocks("# Licenses\nintro\n## Core\n### Crates\nlist\n## UI\nqt", true, true);

    QCOMPARE(blocks.size(), 3);
    QVERIFY(blocks[0].startsWith("#### Licenses\nintro"));
    QVERIFY(blocks[1].startsWith("#### Core\n#### Crates"));
    QVERIFY(blocks[2].startsWith("#### UI"));
}

// A crate license file is half a megabyte, nearly all of it in one "## " section: a linked document is split at
// every heading, but not at a "#" line inside a code block
void testResources::licenseBlocks_linkedDocument_splitsAtEveryHeadingOutsideCode() {
    const QStringList blocks = uc::ui::Resources(QString(), m_legalPath)
                                   .licenseBlocks(
                                       "## All license text\n### MIT\n#### License\n```\n# not a heading\n```\n"
                                       "### ISC\ntext",
                                       true, false);

    QCOMPARE(blocks.size(), 4);
    QVERIFY(blocks[0].startsWith("#### All license text"));
    QVERIFY(blocks[1].startsWith("#### MIT"));
    QVERIFY(blocks[2].startsWith("#### License"));
    QVERIFY(blocks[2].contains("not a heading"));
    QVERIFY(blocks[3].startsWith("#### ISC"));
}

// Qt draws a "#" heading at about twice the size of the text: every level is shown at the text size
void testResources::licenseBlocks_headings_atTextSize() {
    const QStringList blocks =
        uc::ui::Resources(QString(), m_legalPath).licenseBlocks("# One\n## Two\n###### Six\n#hashtag", true, false);

    QCOMPARE(blocks, QStringList({"#### One", "#### Two", "#### Six\n#hashtag"}));
}

// The remote opens no web page and no mail: such links of a license are text, the address after the link text, and
// the Markdown importer must not turn an address in the text into a link either
void testResources::licenseBlocks_webAndMailLinks_areText_data() {
    QTest::addColumn<QString>("markdown");
    QTest::addColumn<QString>("shown");

    QTest::newRow("web link") << "[SQLite](https://www.sqlite.org/): MIT"
                              << "SQLite (https:\\/\\/www\\.sqlite.org/): MIT";
    QTest::newRow("link text is the address") << "[https://example.com](https://example.com)"
                                              << "https:\\/\\/example.com";
    QTest::newRow("mail link") << "[Eric Young](mailto:eay@cryptsoft.com)" << "Eric Young (eay\\@cryptsoft.com)";
    QTest::newRow("web autolink") << "<https://github.com/qt/qt5>" << "\\<https:\\/\\/github.com/qt/qt5\\>";
    QTest::newRow("mail autolink") << "Haoqun Jiang <haoqunjiang+npm@gmail.com>"
                                   << "Haoqun Jiang \\<haoqunjiang+npm\\@gmail.com\\>";
    QTest::newRow("bare address") << "| axios | git+https://github.com/axios/axios.git |"
                                  << "| axios | git+https:\\/\\/github.com/axios/axios.git |";
    QTest::newRow("www") << "(www.openssl.org/)" << "(www\\.openssl.org/)";
    QTest::newRow("odd backtick") << "can`t <https://x.org>" << "can`t \\<https:\\/\\/x.org\\>";
}

void testResources::licenseBlocks_webAndMailLinks_areText() {
    QFETCH(QString, markdown);
    QFETCH(QString, shown);

    QCOMPARE(uc::ui::Resources(QString(), m_legalPath).licenseBlocks(markdown, true, false), QStringList({shown}));
}

// A link the remote can open is underlined: written as an HTML anchor, which Qt underlines
void testResources::licenseBlocks_documentLinks_areAnchors() {
    const QStringList blocks =
        uc::ui::Resources(QString(), m_legalPath)
            .licenseBlocks("- [Crates](remote-core_licenses.md)\n- [MIT License](#MIT) (478)\n- [a & b](b.md)", true,
                           true);

    QCOMPARE(blocks, QStringList({"- <a href=\"remote-core_licenses.md\">Crates</a>\n- <a href=\"#MIT\">MIT "
                                  "License</a> (478)\n- [a & b](b.md)"}));
}

// The overview of a crate license file links its sections as "#MIT"; the files have no other anchors
void testResources::licenseAnchorBlock_findsHeading() {
    uc::ui::Resources resources(QString(), m_legalPath);
    const QStringList blocks = resources.licenseBlocks(
        "## Overview\n- [MIT License](#MIT)\n## All license text\n### MIT\nmit\n### MIT\nagain\n### Apache License 2.0",
        true, false);

    QCOMPARE(resources.licenseAnchorBlock(blocks, "MIT"), 2);
    QCOMPARE(resources.licenseAnchorBlock(blocks, "mit"), 2);
    QCOMPARE(resources.licenseAnchorBlock(blocks, "apache-license-20"), 4);
    QCOMPARE(resources.licenseAnchorBlock(blocks, "ISC"), -1);
}

// A crate license file holds every license text as a code block, which Qt drew in the small fixed-pitch font of the
// system in lines wrapped for a far wider screen: code is text that flows into paragraphs, nothing in it Markdown
void testResources::licenseBlocks_code_isText() {
    const QStringList blocks =
        uc::ui::Resources(QString(), m_legalPath)
            .licenseBlocks(
                "#### License\n```\n   Copyright (c) 2015 *Me*\n\n   See http://x.org\n```\n#### Next\n"
                "Base image is `debian:bookworm-slim`.",
                true, false);

    QCOMPARE(blocks, QStringList({"#### License\n\nCopyright \\(c\\) 2015 \\*Me\\*\n\nSee http\\:\\/\\/x\\.org",
                                  "#### Next\nBase image is debian\\:bookworm\\-slim."}));
}

// The Mesa license in the operating system licenses holds a text table in its code block: flowing into a
// paragraph, its rows ran into one another. Every row keeps its line, the borders are left out.
void testResources::licenseBlocks_textTable_keepsItsRows() {
    const QStringList blocks = uc::ui::Resources(QString(), m_legalPath)
                                   .licenseBlocks(
                                       "#### License\n```\nterms:\n+------+-----+\n| Main | MIT |\n"
                                       "+======+=====+\n| GLX  | SGI |\n+------+-----+\n\nIn general.\n```",
                                       true, false);

    QCOMPARE(blocks, QStringList({"#### License\n\nterms\\:\n\n\\| Main \\| MIT \\|  \n\\| GLX \\| SGI \\|  \n\n"
                                  "In general\\."}));
}

// A Remote 3 needed 2.8 s to lay out the 138 kB code block of the operating system licenses when it came into view:
// code is split into blocks of about 2000 characters where a paragraph ends, also at the "." lines of a Debian
// copyright file, which become paragraph breaks
void testResources::licenseBlocks_longCode_isSplitAtParagraphs() {
    const QString     paragraph = QString("word ").repeated(300);  // 1500 characters
    const QStringList blocks =
        uc::ui::Resources(QString(), m_legalPath)
            .licenseBlocks("#### License\n```\n" + paragraph + "\n\n" + paragraph + "\n .\n" + paragraph + "\n```",
                           true, false);

    QCOMPARE(blocks.size(), 2);
    QVERIFY(blocks[0].startsWith("#### License"));
    QCOMPARE(blocks[0].count(paragraph.trimmed()), 2);
    QCOMPARE(blocks[1], paragraph.trimmed());
}

// A code paragraph longer than twice the block size is split at a line
void testResources::licenseBlocks_longCodeParagraph_isSplitAtALine() {
    const QString     line = QString("word ").repeated(100).trimmed() + "\n";  // 500 characters
    const QStringList blocks =
        uc::ui::Resources(QString(), m_legalPath).licenseBlocks("```\n" + line.repeated(12) + "```", true, false);

    QCOMPARE(blocks.size(), 2);
    for (const QString& block : blocks) {
        QVERIFY(block.size() <= 4 * 1000 + line.size());
    }
}

QTEST_GUILESS_MAIN(testResources)

#include "test_resources.moc"
