// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QQmlEngine>

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
    Q_INVOKABLE QString getLinkContent(const QString& baseDir, const QString& path);

    Q_INVOKABLE QStringList getIconList();
    Q_INVOKABLE QStringList getCustomIconList();

 signals:
    void aboutInfo(QString content, QString baseDir);

 private:
    QJsonObject m_iconList;

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
