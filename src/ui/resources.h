// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDir>
#include <QFile>
#include <QFont>
#include <QFontMetrics>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QQmlEngine>
#include <QScopedPointer>

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

    QScopedPointer<QFontMetrics> m_iconMetrics;

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
