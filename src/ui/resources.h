// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFont>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QQmlEngine>
#include <QRawFont>
#include <QRect>
#include <QSize>
#include <QVariantMap>

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

    /**
     * @brief The size a `custom:` or `ctv:` SVG icon is drawn at in a box of boxWidth x boxHeight.
     *
     * The largest size with the aspect ratio of the root `viewBox` that fits into the box: Qt maps the viewBox onto
     * the size it rasterizes at, so the root width and height are ignored. Without a usable viewBox, the size Qt
     * reports for the file. The size of a file is read once and cached until the file's modification time or size
     * changes. Returns an invalid size for any other icon, a missing or rejected file, a file whose size cannot be read
     * and an empty box: the icon is then decoded at the box size.
     */
    Q_INVOKABLE QSize   getIconSize(const QString& id, int boxWidth, int boxHeight);
    Q_INVOKABLE QString getTvChannelIcon(const QString& id) { return getResource(TvChannelIcon, id); }
    Q_INVOKABLE QString getBackgroundImage(const QString& id) { return getResource(BackgroundImage, id); }
    Q_INVOKABLE QString getSound(const QString& id) { return getResource(Sound, id); }

    /**
     * @brief The background of an activity or remote page, normalised for the screen.
     *
     * Takes the `background` object of a user interface page as the Core-API delivers it and returns a map with
     * `source` (the `file:` URL of the image, empty when there is no background, no or an unusable image identifier or
     * no file), `scale` (`fill`, `fit` or `none`; `fill` for a missing or unknown value), `align` (`top`, `center` or
     * `bottom`; `center` for a missing or unknown value) and `dim` (the scrim opacity 0.0 - 0.8 from the percentage,
     * clamped; 0.4 for a missing or non-numeric value).
     */
    Q_INVOKABLE QVariantMap getPageBackground(const QVariant& background);

    /**
     * @brief The backgrounds of the pages of an activity or remote user interface, grouped into runs.
     *
     * Each page shows its own `background` when it has a non-empty `image`, otherwise defaultBackground (the
     * `background` of the user interface), normalised with getPageBackground(). A run is a maximal sequence of
     * neighbouring pages whose backgrounds have the same `source`, `scale` and `align`; all pages without a source
     * share one key. The pages are circular: the first and the last run are joined when they have the same key.
     *
     * Returns `runs`, in page order, each a map with `first` (its first page), `length` (its number of pages,
     * wrapping from the last to the first page), `source`, `scale`, `align`, `dims` (the scrim of each of its pages),
     * `dim` (the scrim of its first page) and `blend` (whether the scrims of its pages differ), and `runOfPage`, the
     * index of the run of each page.
     */
    Q_INVOKABLE QVariantMap getPageBackgroundRuns(const QVariantList& pages, const QVariant& defaultBackground);

    /**
     * @brief The continuous page position of a PathView with full-width delegates: the index of the centred page,
     * fractional while the pages move, in [0, count). pathOffset is the `offset` of the PathView; 0 without pages.
     */
    Q_INVOKABLE static qreal pageBackgroundPosition(qreal pathOffset, int count);

    /**
     * @brief Where a run of page backgrounds is drawn, in screen widths from the screen's left edge.
     *
     * 0 while position is within the run's pages first .. first + length - 1 (circular): the background stays still.
     * Otherwise the offset of the run's nearer end page, the way the PathView places that page: negative when the run
     * lies to the left, positive to the right. A value of 1 or more in either direction is off the screen.
     */
    Q_INVOKABLE static qreal pageBackgroundRunOffset(int first, int length, qreal position, int count);

    /**
     * @brief The scrim of a run at a page position: within the run, the scrims of the two pages around the position
     * blended linearly, otherwise the scrim of the run's nearer end page. dims holds the scrim of each page of the
     * run, starting at its first page.
     */
    Q_INVOKABLE static qreal pageBackgroundRunDim(const QVariantList& dims, int first, qreal position, int count);

    /**
     * @brief Whether a run decodes its image: the run of the current page and the runs before and after it.
     */
    Q_INVOKABLE static bool pageBackgroundRunLoaded(int run, int currentRun, int runCount);

    /**
     * @brief The part of a background image a main page header uses.
     *
     * A header shows a 480 x 275 image. Of an image with a taller aspect ratio, such as the full-screen sizes 480 x 800
     * and 480 x 854, only its top part with the aspect ratio of 480 x 275 is used: the returned rectangle in image
     * pixels. Returns an empty rectangle for an image that is not taller, an empty identifier, a missing file and a
     * file whose size cannot be read: the whole image is used then. Only the image header is read; the size of a file
     * is cached until its modification time or size changes.
     */
    Q_INVOKABLE QRect   getBackgroundImageClip(const QString& id);
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
     * @brief Returns the glyph of the placeholder icon, a question mark in a circle.
     *
     * Shown in place of an icon file that exists but cannot be used: one the checks reject or one
     * the image loader cannot decode.
     */
    Q_INVOKABLE QString getIconPlaceholder();

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

    /**
     * @brief Returns the `file:` URL of a custom or TV channel icon file, or what to show instead.
     *
     * A missing file resolves to an empty string. A file that exists but cannot be used, an SVG
     * above the size limit or a compressed one, resolves to the placeholder glyph and is logged.
     */
    QString getIconFile(ResourceType type, const QString& id, const QString& name);

    // the viewBox size of an SVG icon file, with the file's modification time and size it was read from
    struct SvgIconSize {
        QDateTime modified;
        qint64    fileSize = -1;
        QSizeF    size;
    };
    QHash<QString, SvgIconSize> m_svgIconSizes;

    // the size of a background image file, with the file's modification time and size it was read from
    struct BackgroundImageSize {
        QDateTime modified;
        qint64    fileSize = -1;
        QSize     size;
    };
    QHash<QString, BackgroundImageSize> m_backgroundImageSizes;

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
