// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDir>
#include <QFile>
#include <QFont>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QQmlEngine>
#include <QRawFont>

namespace uc {
namespace ui {

class Resources : public QObject {
    Q_OBJECT

 public:
    explicit Resources(const QString& resourcePath, const QString& legalPath, QObject* parent = nullptr);
    ~Resources();

    enum ResourceType { Icon, TvChannelIcon, BackgroundImage, Sound };
    Q_ENUM(ResourceType)

    enum AboutType { Regulatory, Terms, Warranty, Licenses };
    Q_ENUM(AboutType)

    Q_INVOKABLE QString getIcon(const QString& id, const QString& suffix = QString());
    Q_INVOKABLE QString getTvChannelIcon(const QString& id) { return getResource(TvChannelIcon, id); }
    Q_INVOKABLE QString getBackgroundImage(const QString& id) { return getResource(BackgroundImage, id); }
    Q_INVOKABLE QString getSound(const QString& id) { return getResource(Sound, id); }
    Q_INVOKABLE void    getAboutInfo(int type);
    Q_INVOKABLE void    getLinkContent(const QString& baseDir, const QString& path);

    /**
     * @brief Whether the Licenses page shows a linked file as Markdown: its name ends in ".md".
     */
    Q_INVOKABLE bool isMarkdownFile(const QString& link) const;

    /**
     * @brief The blocks the Licenses page shows of a document, one text item each.
     *
     * Markdown is split outside code blocks at its "## " headings for the overview and at every heading for a
     * linked document, which can be hundreds of kB, and prepared for the screen: every heading at the size of the
     * text, only the links the remote can open: a link to another document or to a heading, underlined, while a web
     * or mail address is text, and code as text that flows into paragraphs. Any other text gives one block per line.
     */
    Q_INVOKABLE QStringList licenseBlocks(const QString& content, bool markdown, bool overview) const;

    /**
     * @brief The block a link such as "#MIT" leads to, or -1: the first one whose heading reads "MIT", as the license
     * files name their sections, or whose heading has the anchor a Markdown viewer gives it.
     */
    Q_INVOKABLE int licenseAnchorBlock(const QStringList& blocks, const QString& anchor) const;

    /**
     * @brief The links of the blocks licenseBlocks() made, in reading order, for the d-pad: each a map of the block,
     * the link's index within the block and its target.
     */
    Q_INVOKABLE QVariantList licenseLinks(const QStringList& blocks) const;

    /**
     * @brief The block with its link at the given index drawn on the given background, the selection fill.
     */
    Q_INVOKABLE QString licenseBlockWithSelection(const QString& block, int index, const QString& background) const;

    Q_INVOKABLE QStringList getIconList();
    Q_INVOKABLE QStringList getCustomIconList();

    /**
     * @brief Sets the font family the icon glyphs are rendered with.
     *
     * Enables the icon fallback: an icon the embedded font cannot draw is replaced by its
     * mapped alternative, or by the placeholder icon. Called once at startup with the family
     * of the embedded icon font, which differs between the Free and the Pro edition of the
     * icon set (see docs/icon-font.md).
     */
    void setIconFont(const QString& family);

 signals:
    void aboutInfo(QString content, QString baseDir);

 private:
    QJsonObject m_iconList;
    QJsonObject m_iconFallback;
    QString     m_iconPlaceholder;

    // the icon font alone, without Qt's fallback fonts; invalid until setIconFont()
    QRawFont m_iconFont;

    /**
     * @brief Returns the icon glyph for a mapped icon name, or an empty string.
     *
     * An icon the embedded font cannot draw is replaced by its entry in the fallback mapping
     * and, if that does not help either, by the placeholder icon.
     */
    QString getIconGlyph(const QString& name);
    bool    canRenderGlyph(const QString& glyph) const;

    QString                      m_resourcePath;
    QHash<ResourceType, QString> m_resourcePaths = {{Icon, m_resourcePath + "/Icon/"},
                                                    {TvChannelIcon, m_resourcePath + "/TvChannelIcon/"},
                                                    {BackgroundImage, m_resourcePath + "/BackgroundImage/"},
                                                    {Sound, m_resourcePath + "/Sound/"}};

    QString m_legalPath;

 private:
    QString getResource(ResourceType type, const QString& id);
};
}  // namespace ui
}  // namespace uc
