// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>
#include <cmath>

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
    void licenseLinks_inReadingOrder();
    void licenseBlockWithSelection_drawsOneLink();

    void pageBackground_none_givesDefaults_data();
    void pageBackground_none_givesDefaults();
    void pageBackground_image_resolvesFile();
    void pageBackground_unusableImage_givesNoSource_data();
    void pageBackground_unusableImage_givesNoSource();
    void pageBackground_knownSettings_passThrough_data();
    void pageBackground_knownSettings_passThrough();
    void pageBackground_unknownSettings_giveDefaults();
    void pageBackground_dim_data();
    void pageBackground_dim();

    void pageBackgroundRuns_noPages();
    void pageBackgroundRuns_singlePage();
    void pageBackgroundRuns_withoutDefault_keepsOwnBackgrounds();
    void pageBackgroundRuns_default_fillsPagesWithoutOwn();
    void pageBackgroundRuns_emptyOwnImage_fallsBackToDefault();
    void pageBackgroundRuns_missingOwnFile_staysBlack();
    void pageBackgroundRuns_allEqual_oneRun();
    void pageBackgroundRuns_dimOnly_oneBlendingRun();
    void pageBackgroundRuns_otherAlign_twoRuns();
    void pageBackgroundRuns_noImage_oneKeyWhateverTheMode();
    void pageBackgroundRuns_unknownSettings_countAsDefaults();
    void pageBackgroundPosition_data();
    void pageBackgroundPosition();
    void pageBackgroundRunOffset_data();
    void pageBackgroundRunOffset();
    void pageBackgroundRunOffset_singlePage_followsPathView();
    void pageBackgroundRunDim_data();
    void pageBackgroundRunDim();
    void pageBackgroundRunLoaded_data();
    void pageBackgroundRunLoaded();

    void backgroundImageClip_data();
    void backgroundImageClip();
    void backgroundImageClip_unusable_data();
    void backgroundImageClip_unusable();
    void backgroundImageClip_replacedFile_isProbedAgain();

 private:
    static void writeFile(const QString& path, const QByteArray& content);
    QString     baseUrl(const QString& subDir) const;

    static void writeImage(const QString& path, int width, int height, const char* format);

    // a page of a user interface with the given background, or without one for an invalid QVariant
    static QVariantMap page(const QVariant& background = QVariant());
    // the `runs` of getPageBackgroundRuns() as "first+length" strings, for example "3+2"
    static QStringList runExtents(const QVariantMap& result);

    QTemporaryDir m_root;
    QString       m_legalPath;
    QString       m_resourcePath;
};

void testResources::writeImage(const QString& path, int width, int height, const char* format) {
    QVERIFY(QDir().mkpath(QFileInfo(path).absolutePath()));
    QImage image(width, height, QImage::Format_RGB32);
    image.fill(Qt::darkGray);
    QVERIFY(image.save(path, format));
}

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

    m_resourcePath = m_root.path() + "/resources";
    writeImage(m_resourcePath + "/BackgroundImage/bg.png", 480, 800, "PNG");
    writeImage(m_resourcePath + "/BackgroundImage/other.png", 480, 854, "PNG");
}

QVariantMap testResources::page(const QVariant& background) {
    QVariantMap result{{"page_id", "p"}, {"items", QVariantList()}};
    if (background.isValid()) {
        result.insert("background", background);
    }
    return result;
}

QStringList testResources::runExtents(const QVariantMap& result) {
    QStringList extents;
    for (const QVariant& run : result.value("runs").toList()) {
        const QVariantMap values = run.toMap();
        extents.append(QString("%1+%2").arg(values.value("first").toInt()).arg(values.value("length").toInt()));
    }
    return extents;
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
    const QStringList blocks = uc::ui::Resources(QString(), m_legalPath)
                                   .licenseBlocks(
                                       "- [Crates](remote-core_licenses.md)\n- [MIT License](#MIT) (478)\n"
                                       "- [Bang & Olufsen](integrations/bangolufsen.md)\n- [a <b>](c.md)",
                                       true, true);

    QCOMPARE(blocks, QStringList({"- <a href=\"remote-core_licenses.md\">Crates</a>\n- <a href=\"#MIT\">MIT "
                                  "License</a> (478)\n- <a href=\"integrations/bangolufsen.md\">Bang & Olufsen</a>\n"
                                  "- [a <b>](c.md)"}));
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

// The licenses could only be opened by touch: the d-pad walks the links in reading order
void testResources::licenseLinks_inReadingOrder() {
    uc::ui::Resources resources(QString(), m_legalPath);
    const QStringList blocks = resources.licenseBlocks(
        "# Licenses\n- [remote-ui](remote-ui/README.md)\n- [SQLite](https://www.sqlite.org/)\n## Remote 3\n"
        "- [Android TV](integrations/androidtv.md) and [Roku](integrations/roku.md)",
        true, true);

    const QVariantList links = resources.licenseLinks(blocks);
    QCOMPARE(links.size(), 3);
    QCOMPARE(links[0].toMap(), QVariantMap({{"block", 0}, {"index", 0}, {"link", "remote-ui/README.md"}}));
    QCOMPARE(links[1].toMap(), QVariantMap({{"block", 1}, {"index", 0}, {"link", "integrations/androidtv.md"}}));
    QCOMPARE(links[2].toMap(), QVariantMap({{"block", 1}, {"index", 1}, {"link", "integrations/roku.md"}}));
}

// The selected link is drawn on the selection fill; the other links of the block stay as they are
void testResources::licenseBlockWithSelection_drawsOneLink() {
    uc::ui::Resources resources(QString(), m_legalPath);
    const QString     block = "<a href=\"a.md\">A</a> and <a href=\"b.md\">B</a>";

    QCOMPARE(resources.licenseBlockWithSelection(block, 1, "#595959"),
             QString("<a href=\"a.md\">A</a> and <a href=\"b.md\" style=\"background-color: #595959\">B</a>"));
    QCOMPARE(resources.licenseBlockWithSelection(block, 2, "#595959"), block);
}

void testResources::pageBackground_none_givesDefaults_data() {
    QTest::addColumn<QVariant>("background");

    QTest::newRow("missing") << QVariant();
    QTest::newRow("empty map") << QVariant(QVariantMap());
}

void testResources::pageBackground_none_givesDefaults() {
    QFETCH(QVariant, background);
    uc::ui::Resources resources(m_resourcePath, m_legalPath);

    const QVariantMap result = resources.getPageBackground(background);

    QCOMPARE(result.value("source").toString(), QString());
    QCOMPARE(result.value("scale").toString(), QStringLiteral("fill"));
    QCOMPARE(result.value("align").toString(), QStringLiteral("center"));
    QCOMPARE(result.value("dim").toReal(), 0.4);
}

void testResources::pageBackground_image_resolvesFile() {
    uc::ui::Resources resources(m_resourcePath, m_legalPath);

    const QVariantMap result = resources.getPageBackground(QVariantMap{{"image", "custom:bg.png"}});

    QCOMPARE(result.value("source").toString(), "file:" + m_resourcePath + "/BackgroundImage/bg.png");
    QCOMPARE(result.value("scale").toString(), QStringLiteral("fill"));
    QCOMPARE(result.value("align").toString(), QStringLiteral("center"));
    QCOMPARE(result.value("dim").toReal(), 0.4);
}

void testResources::pageBackground_unusableImage_givesNoSource_data() {
    QTest::addColumn<QVariant>("image");

    QTest::newRow("empty") << QVariant(QString());
    QTest::newRow("missing file") << QVariant("custom:missing.png");
    QTest::newRow("no colon") << QVariant("bg.png");
    QTest::newRow("no file name") << QVariant("custom:");
}

void testResources::pageBackground_unusableImage_givesNoSource() {
    QFETCH(QVariant, image);
    uc::ui::Resources resources(m_resourcePath, m_legalPath);

    const QVariantMap result = resources.getPageBackground(QVariantMap{{"image", image}, {"dim", 60}});

    QCOMPARE(result.value("source").toString(), QString());
    QCOMPARE(result.value("dim").toReal(), 0.6);
}

void testResources::pageBackground_knownSettings_passThrough_data() {
    QTest::addColumn<QString>("scale");
    QTest::addColumn<QString>("align");

    for (const char* scale : {"fill", "fit", "none"}) {
        for (const char* align : {"top", "center", "bottom"}) {
            QTest::addRow("%s %s", scale, align) << QString(scale) << QString(align);
        }
    }
}

void testResources::pageBackground_knownSettings_passThrough() {
    QFETCH(QString, scale);
    QFETCH(QString, align);
    uc::ui::Resources resources(m_resourcePath, m_legalPath);

    const QVariantMap result =
        resources.getPageBackground(QVariantMap{{"image", "custom:bg.png"}, {"scale", scale}, {"align", align}});

    QCOMPARE(result.value("scale").toString(), scale);
    QCOMPARE(result.value("align").toString(), align);
}

void testResources::pageBackground_unknownSettings_giveDefaults() {
    uc::ui::Resources resources(m_resourcePath, m_legalPath);

    const QVariantMap result =
        resources.getPageBackground(QVariantMap{{"image", "custom:bg.png"}, {"scale", "tile"}, {"align", "left"}});

    QCOMPARE(result.value("scale").toString(), QStringLiteral("fill"));
    QCOMPARE(result.value("align").toString(), QStringLiteral("center"));
}

void testResources::pageBackground_dim_data() {
    QTest::addColumn<QVariant>("dim");
    QTest::addColumn<qreal>("expected");

    QTest::newRow("0") << QVariant(0) << 0.0;
    QTest::newRow("60") << QVariant(60) << 0.6;
    QTest::newRow("80") << QVariant(80) << 0.8;
    QTest::newRow("below range") << QVariant(-5) << 0.0;
    QTest::newRow("above range") << QVariant(95) << 0.8;
    QTest::newRow("double") << QVariant(25.0) << 0.25;
    QTest::newRow("not a number") << QVariant("abc") << 0.4;
    QTest::newRow("missing") << QVariant() << 0.4;
}

void testResources::pageBackground_dim() {
    QFETCH(QVariant, dim);
    QFETCH(qreal, expected);
    uc::ui::Resources resources(m_resourcePath, m_legalPath);

    QVariantMap background{{"image", "custom:bg.png"}};
    if (dim.isValid()) {
        background.insert("dim", dim);
    }

    QCOMPARE(resources.getPageBackground(background).value("dim").toReal(), expected);
}

void testResources::pageBackgroundRuns_noPages() {
    uc::ui::Resources resources(m_resourcePath, m_legalPath);

    const QVariantMap result = resources.getPageBackgroundRuns(QVariantList(), QVariantMap{{"image", "custom:bg.png"}});

    QVERIFY(result.value("runs").toList().isEmpty());
    QVERIFY(result.value("runOfPage").toList().isEmpty());
}

void testResources::pageBackgroundRuns_singlePage() {
    uc::ui::Resources resources(m_resourcePath, m_legalPath);

    const QVariantMap result =
        resources.getPageBackgroundRuns({page(QVariantMap{{"image", "custom:bg.png"}, {"dim", 20}})}, QVariant());

    QCOMPARE(runExtents(result), QStringList{"0+1"});
    const QVariantMap run = result.value("runs").toList().first().toMap();
    QCOMPARE(run.value("source").toString(), "file:" + m_resourcePath + "/BackgroundImage/bg.png");
    QCOMPARE(run.value("dim").toReal(), 0.2);
    QCOMPARE(run.value("blend").toBool(), false);
    QCOMPARE(result.value("runOfPage").toList(), QVariantList{0});
}

void testResources::pageBackgroundRuns_withoutDefault_keepsOwnBackgrounds() {
    uc::ui::Resources resources(m_resourcePath, m_legalPath);
    const QVariantMap bg{{"image", "custom:bg.png"}};

    // the last page continues into the first one: one run of pages 3 and 0
    const QVariantMap result =
        resources.getPageBackgroundRuns({page(bg), page(), page(QVariant()), page(bg)}, QVariant());

    QCOMPARE(runExtents(result), (QStringList{"1+2", "3+2"}));
    const QVariantList runs = result.value("runs").toList();
    QCOMPARE(runs.at(0).toMap().value("source").toString(), QString());
    QCOMPARE(runs.at(1).toMap().value("source").toString(), "file:" + m_resourcePath + "/BackgroundImage/bg.png");
    QCOMPARE(runs.at(1).toMap().value("dims").toList(), (QVariantList{0.4, 0.4}));
    QCOMPARE(result.value("runOfPage").toList(), (QVariantList{1, 0, 0, 1}));
}

void testResources::pageBackgroundRuns_default_fillsPagesWithoutOwn() {
    uc::ui::Resources resources(m_resourcePath, m_legalPath);
    const QVariantMap defaultBackground{{"image", "custom:bg.png"}, {"dim", 30}};

    const QVariantMap result = resources.getPageBackgroundRuns(
        {page(), page(QVariantMap{{"image", "custom:other.png"}, {"scale", "fit"}}), page(QVariant())},
        defaultBackground);

    QCOMPARE(runExtents(result), (QStringList{"1+1", "2+2"}));
    const QVariantList runs = result.value("runs").toList();
    QCOMPARE(runs.at(0).toMap().value("source").toString(), "file:" + m_resourcePath + "/BackgroundImage/other.png");
    QCOMPARE(runs.at(0).toMap().value("scale").toString(), QStringLiteral("fit"));
    QCOMPARE(runs.at(0).toMap().value("dim").toReal(), 0.4);
    QCOMPARE(runs.at(1).toMap().value("source").toString(), "file:" + m_resourcePath + "/BackgroundImage/bg.png");
    QCOMPARE(runs.at(1).toMap().value("dim").toReal(), 0.3);
    QCOMPARE(result.value("runOfPage").toList(), (QVariantList{1, 0, 1}));
}

void testResources::pageBackgroundRuns_emptyOwnImage_fallsBackToDefault() {
    uc::ui::Resources resources(m_resourcePath, m_legalPath);

    const QVariantMap result = resources.getPageBackgroundRuns(
        {page(QVariantMap{{"image", ""}, {"dim", 70}}), page(QVariantMap())}, QVariantMap{{"image", "custom:bg.png"}});

    QCOMPARE(runExtents(result), QStringList{"0+2"});
    const QVariantMap run = result.value("runs").toList().first().toMap();
    QCOMPARE(run.value("source").toString(), "file:" + m_resourcePath + "/BackgroundImage/bg.png");
    // the settings of the page's own empty background are not used
    QCOMPARE(run.value("dims").toList(), (QVariantList{0.4, 0.4}));
}

void testResources::pageBackgroundRuns_missingOwnFile_staysBlack() {
    uc::ui::Resources resources(m_resourcePath, m_legalPath);

    const QVariantMap result = resources.getPageBackgroundRuns(
        {page(QVariantMap{{"image", "custom:missing.png"}}), page()}, QVariantMap{{"image", "custom:bg.png"}});

    QCOMPARE(runExtents(result), (QStringList{"0+1", "1+1"}));
    QCOMPARE(result.value("runs").toList().at(0).toMap().value("source").toString(), QString());
}

void testResources::pageBackgroundRuns_allEqual_oneRun() {
    uc::ui::Resources resources(m_resourcePath, m_legalPath);

    const QVariantMap result = resources.getPageBackgroundRuns(
        {page(), page(), page(QVariantMap{{"image", "custom:bg.png"}})}, QVariantMap{{"image", "custom:bg.png"}});

    QCOMPARE(runExtents(result), QStringList{"0+3"});
    const QVariantMap run = result.value("runs").toList().first().toMap();
    QCOMPARE(run.value("blend").toBool(), false);
    QCOMPARE(run.value("dim").toReal(), 0.4);
    QCOMPARE(result.value("runOfPage").toList(), (QVariantList{0, 0, 0}));
}

void testResources::pageBackgroundRuns_dimOnly_oneBlendingRun() {
    uc::ui::Resources resources(m_resourcePath, m_legalPath);

    const QVariantMap result =
        resources.getPageBackgroundRuns({page(QVariantMap{{"image", "custom:bg.png"}, {"dim", 20}}),
                                         page(QVariantMap{{"image", "custom:bg.png"}, {"dim", 60}})},
                                        QVariant());

    QCOMPARE(runExtents(result), QStringList{"0+2"});
    const QVariantMap run = result.value("runs").toList().first().toMap();
    QCOMPARE(run.value("blend").toBool(), true);
    QCOMPARE(run.value("dims").toList(), (QVariantList{0.2, 0.6}));
}

void testResources::pageBackgroundRuns_otherAlign_twoRuns() {
    uc::ui::Resources resources(m_resourcePath, m_legalPath);

    const QVariantMap result =
        resources.getPageBackgroundRuns({page(QVariantMap{{"image", "custom:bg.png"}, {"align", "top"}}),
                                         page(QVariantMap{{"image", "custom:bg.png"}, {"align", "bottom"}})},
                                        QVariant());

    QCOMPARE(runExtents(result), (QStringList{"0+1", "1+1"}));
    QCOMPARE(result.value("runOfPage").toList(), (QVariantList{0, 1}));
}

void testResources::pageBackgroundRuns_noImage_oneKeyWhateverTheMode() {
    uc::ui::Resources resources(m_resourcePath, m_legalPath);

    const QVariantMap result = resources.getPageBackgroundRuns(
        {page(QVariantMap{{"image", "custom:missing.png"}, {"scale", "fit"}, {"align", "top"}, {"dim", 80}}), page(),
         page(QVariantMap{{"image", "custom:bg.png"}})},
        QVariant());

    QCOMPARE(runExtents(result), (QStringList{"0+2", "2+1"}));
    const QVariantMap run = result.value("runs").toList().first().toMap();
    QCOMPARE(run.value("source").toString(), QString());
    QCOMPARE(run.value("blend").toBool(), false);
}

void testResources::pageBackgroundRuns_unknownSettings_countAsDefaults() {
    uc::ui::Resources resources(m_resourcePath, m_legalPath);

    const QVariantMap result = resources.getPageBackgroundRuns(
        {page(QVariantMap{{"image", "custom:bg.png"}, {"scale", "tile"}, {"align", "left"}}),
         page(QVariantMap{{"image", "custom:bg.png"}})},
        QVariantMap{{"image", "custom:bg.png"}, {"scale", 5}});

    QCOMPARE(runExtents(result), QStringList{"0+2"});
    const QVariantMap run = result.value("runs").toList().first().toMap();
    QCOMPARE(run.value("scale").toString(), QStringLiteral("fill"));
    QCOMPARE(run.value("align").toString(), QStringLiteral("center"));
}

void testResources::pageBackgroundPosition_data() {
    QTest::addColumn<qreal>("offset");
    QTest::addColumn<int>("count");
    QTest::addColumn<qreal>("expected");

    // measured with a PathView of the activity and remote screens
    QTest::newRow("first page") << 0.0 << 4 << 0.0;
    QTest::newRow("first page moving right") << 0.25 << 4 << 3.75;
    QTest::newRow("last page") << 1.0 << 4 << 3.0;
    QTest::newRow("first page moving left") << 3.75 << 4 << 0.25;
    QTest::newRow("between pages 1 and 2") << 2.5 << 4 << 1.5;
    QTest::newRow("two pages") << 0.3 << 2 << 1.7;
    QTest::newRow("one page") << 0.0 << 1 << 0.0;
    QTest::newRow("no pages") << 0.0 << 0 << 0.0;
}

void testResources::pageBackgroundPosition() {
    QFETCH(qreal, offset);
    QFETCH(int, count);
    QFETCH(qreal, expected);

    QCOMPARE(uc::ui::Resources::pageBackgroundPosition(offset, count), expected);
}

void testResources::pageBackgroundRunOffset_data() {
    QTest::addColumn<int>("first");
    QTest::addColumn<int>("length");
    QTest::addColumn<qreal>("position");
    QTest::addColumn<int>("count");
    QTest::addColumn<qreal>("expected");

    QTest::newRow("on the first page of the run") << 1 << 2 << 1.0 << 4 << 0.0;
    QTest::newRow("between the pages of the run") << 1 << 2 << 1.5 << 4 << 0.0;
    QTest::newRow("on the last page of the run") << 1 << 2 << 2.0 << 4 << 0.0;
    QTest::newRow("leaving the run to the right") << 1 << 2 << 2.25 << 4 << -0.25;
    QTest::newRow("coming from the left") << 1 << 2 << 0.75 << 4 << 0.25;
    QTest::newRow("far left") << 1 << 2 << 3.5 << 4 << -1.5;
    QTest::newRow("single page, moving left") << 0 << 1 << 0.25 << 4 << -0.25;
    QTest::newRow("last to first page") << 0 << 1 << 3.75 << 4 << 0.25;
    QTest::newRow("first to last page") << 3 << 1 << 0.25 << 4 << -1.25;
    QTest::newRow("last page from the first") << 3 << 1 << 3.75 << 4 << -0.75;
    QTest::newRow("wrapping run, inside") << 3 << 2 << 3.5 << 4 << 0.0;
    QTest::newRow("wrapping run, leaving") << 3 << 2 << 0.5 << 4 << -0.5;
    QTest::newRow("wrapping run, coming") << 3 << 2 << 2.5 << 4 << 0.5;
    QTest::newRow("two pages, page 1 coming") << 1 << 1 << 0.3 << 2 << 0.7;
    QTest::newRow("two pages, page 0 leaving") << 0 << 1 << 0.3 << 2 << -0.3;
    QTest::newRow("two pages, page 1 leaving") << 1 << 1 << 1.7 << 2 << -0.7;
    QTest::newRow("two pages, page 0 coming") << 0 << 1 << 1.7 << 2 << 0.3;
    QTest::newRow("whole circle") << 2 << 3 << 1.5 << 3 << 0.0;
    QTest::newRow("one page") << 0 << 1 << 0.0 << 1 << 0.0;
    QTest::newRow("no pages") << 0 << 1 << 0.0 << 0 << 0.0;
}

void testResources::pageBackgroundRunOffset() {
    QFETCH(int, first);
    QFETCH(int, length);
    QFETCH(qreal, position);
    QFETCH(int, count);
    QFETCH(qreal, expected);

    QCOMPARE(uc::ui::Resources::pageBackgroundRunOffset(first, length, position, count) + 10, expected + 10);
}

void testResources::pageBackgroundRunOffset_singlePage_followsPathView() {
    // A single-page run moves exactly like its PathView delegate, which is drawn at
    // ((page - position + count / 2) mod count) - count / 2 screen widths (measured with Qt 5.15.19).
    for (int count = 2; count <= 5; ++count) {
        for (int page = 0; page < count; ++page) {
            for (int step = 0; step < count * 20; ++step) {
                const qreal position = step / 20.0;
                qreal       delegate = std::fmod(page - position + count / 2.0, qreal(count));
                if (delegate < 0) {
                    delegate += count;
                }
                delegate -= count / 2.0;

                const qreal offset = uc::ui::Resources::pageBackgroundRunOffset(page, 1, position, count);
                if (qAbs(delegate) < 1) {
                    QVERIFY2(qFuzzyCompare(offset + 10, delegate + 10),
                             qPrintable(QString("count %1 page %2 position %3: %4 instead of %5")
                                            .arg(count)
                                            .arg(page)
                                            .arg(position)
                                            .arg(offset)
                                            .arg(delegate)));
                } else {
                    QVERIFY(qAbs(offset) >= 1);
                }
            }
        }
    }
}

void testResources::pageBackgroundRunDim_data() {
    QTest::addColumn<QVariantList>("dims");
    QTest::addColumn<int>("first");
    QTest::addColumn<qreal>("position");
    QTest::addColumn<int>("count");
    QTest::addColumn<qreal>("expected");

    const QVariantList three{0.2, 0.6, 0.8};
    QTest::newRow("first page") << three << 1 << 1.0 << 5 << 0.2;
    QTest::newRow("halfway") << three << 1 << 1.5 << 5 << 0.4;
    QTest::newRow("quarter") << three << 1 << 2.25 << 5 << 0.65;
    QTest::newRow("last page") << three << 1 << 3.0 << 5 << 0.8;
    QTest::newRow("after the run") << three << 1 << 3.5 << 5 << 0.8;
    QTest::newRow("before the run") << three << 1 << 0.5 << 5 << 0.2;
    QTest::newRow("wrapping run") << QVariantList{0.2, 0.6} << 4 << 4.5 << 5 << 0.4;
    QTest::newRow("wrapping run, after it") << QVariantList{0.2, 0.6} << 4 << 0.5 << 5 << 0.6;
    QTest::newRow("whole circle, wrap") << QVariantList{0.2, 0.6} << 0 << 1.5 << 2 << 0.4;
    QTest::newRow("whole circle, near the first") << QVariantList{0.2, 0.6} << 0 << 1.75 << 2 << 0.3;
    QTest::newRow("no dims") << QVariantList() << 0 << 0.0 << 2 << 0.0;
}

void testResources::pageBackgroundRunDim() {
    QFETCH(QVariantList, dims);
    QFETCH(int, first);
    QFETCH(qreal, position);
    QFETCH(int, count);
    QFETCH(qreal, expected);

    QCOMPARE(uc::ui::Resources::pageBackgroundRunDim(dims, first, position, count) + 10, expected + 10);
}

void testResources::pageBackgroundRunLoaded_data() {
    QTest::addColumn<int>("run");
    QTest::addColumn<int>("currentRun");
    QTest::addColumn<int>("runCount");
    QTest::addColumn<bool>("expected");

    QTest::newRow("only run") << 0 << 0 << 1 << true;
    QTest::newRow("other of two") << 1 << 0 << 2 << true;
    QTest::newRow("current") << 2 << 2 << 4 << true;
    QTest::newRow("next") << 3 << 2 << 4 << true;
    QTest::newRow("previous") << 1 << 2 << 4 << true;
    QTest::newRow("opposite") << 0 << 2 << 4 << false;
    QTest::newRow("previous across the wrap") << 3 << 0 << 4 << true;
    QTest::newRow("next across the wrap") << 0 << 3 << 4 << true;
    QTest::newRow("two away") << 3 << 1 << 5 << false;
    QTest::newRow("no runs") << 0 << 0 << 0 << false;
}

void testResources::pageBackgroundRunLoaded() {
    QFETCH(int, run);
    QFETCH(int, currentRun);
    QFETCH(int, runCount);
    QFETCH(bool, expected);

    QCOMPARE(uc::ui::Resources::pageBackgroundRunLoaded(run, currentRun, runCount), expected);
}

void testResources::backgroundImageClip_data() {
    QTest::addColumn<int>("height");
    QTest::addColumn<QByteArray>("format");
    QTest::addColumn<QRect>("expected");

    for (const char* format : {"PNG", "JPG"}) {
        QTest::addRow("480x275 %s", format) << 275 << QByteArray(format) << QRect();
        QTest::addRow("480x800 %s", format) << 800 << QByteArray(format) << QRect(0, 0, 480, 275);
        QTest::addRow("480x854 %s", format) << 854 << QByteArray(format) << QRect(0, 0, 480, 275);
    }
}

void testResources::backgroundImageClip() {
    QFETCH(int, height);
    QFETCH(QByteArray, format);
    QFETCH(QRect, expected);

    const QString name = QString("clip-%1.%2").arg(height).arg(QString(format).toLower());
    writeImage(m_resourcePath + "/BackgroundImage/" + name, 480, height, format.constData());
    uc::ui::Resources resources(m_resourcePath, m_legalPath);

    QCOMPARE(resources.getBackgroundImageClip("custom:" + name), expected);
}

void testResources::backgroundImageClip_unusable_data() {
    QTest::addColumn<QString>("id");

    QTest::newRow("empty id") << QString();
    QTest::newRow("missing file") << QStringLiteral("custom:missing.png");
    QTest::newRow("not an image") << QStringLiteral("custom:garbage.png");
}

void testResources::backgroundImageClip_unusable() {
    QFETCH(QString, id);
    writeFile(m_resourcePath + "/BackgroundImage/garbage.png", "this is not an image");
    uc::ui::Resources resources(m_resourcePath, m_legalPath);

    QCOMPARE(resources.getBackgroundImageClip(id), QRect());
}

void testResources::backgroundImageClip_replacedFile_isProbedAgain() {
    const QString path = m_resourcePath + "/BackgroundImage/replaced.png";
    writeImage(path, 480, 800, "PNG");
    uc::ui::Resources resources(m_resourcePath, m_legalPath);

    QCOMPARE(resources.getBackgroundImageClip("custom:replaced.png"), QRect(0, 0, 480, 275));

    // another size, with a modification time that differs even on a coarse file system clock
    const QDateTime modified = QFileInfo(path).lastModified();
    writeImage(path, 480, 275, "PNG");
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadWrite));
    QVERIFY(file.setFileTime(modified.addSecs(10), QFileDevice::FileModificationTime));
    file.close();
    QCOMPARE(resources.getBackgroundImageClip("custom:replaced.png"), QRect());
}

QTEST_GUILESS_MAIN(testResources)

#include "test_resources.moc"
