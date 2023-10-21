// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "resources.h"

#include "../logging.h"
#include "../util.h"

namespace uc {
namespace ui {

Resources::Resources(const QString& resourcePath, const QString& legalPath, QObject* parent)
    : QObject(parent), m_resourcePath(resourcePath), m_legalPath(legalPath) {
    QFile file(":icon-mapping.json");

    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(lcResources()) << "Cannot open icon mapping file";
    } else {
        QString data = file.readAll();

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data.toUtf8());
        m_iconList = jsonDoc.object();

        qCDebug(lcResources()) << "Icon mapping file loaded";
    }

    qmlRegisterUncreatableType<Resources>("ResourceTypes", 1, 0, "ResourceTypes", "Enum is not a type");
}

Resources::~Resources() {}

QString Resources::getIcon(const QString& id, const QString& suffix) {
    QString _id;
    QString ext;

    if (id.contains(".png") || id.contains(".jpg")) {
        _id = id.chopped(4);
        ext = id.right(4);
    } else if (id.contains(".jpeg")) {
        _id = id.chopped(5);
        ext = id.right(5);
    } else {
        _id = id;
    }

    QString icon;

    if (!suffix.isEmpty()) {
        icon = getResource(Icon, _id + "-" + suffix.toLower() + ext);
    }

    if (icon.isEmpty()) {
        icon = getResource(Icon, _id + ext);
    }

    return icon;
}

void Resources::getAboutInfo(int type) {
    QFile     file;
    AboutType typeEnum = static_cast<AboutType>(type);

    QDir directory(QString(m_legalPath + "/" + Util::convertEnumToString(typeEnum).toLower()));

    qCDebug(lcResources()) << "Current directory" << directory.absolutePath();

    switch (typeEnum) {
        case AboutType::Regulatory:
        case AboutType::Terms:
        case AboutType::Warranty: {
            QStringList fileNames = directory.entryList(QStringList() << "*.html"
                                                                      << "*.md",
                                                        QDir::Files | QDir::NoDot | QDir::NoDotAndDotDot);

            qCDebug(lcResources()) << fileNames;

            if (!fileNames.isEmpty()) {
                file.setFileName(directory.absoluteFilePath(fileNames.first()));
            } else {
                qCWarning(lcResources()) << "Cannot open file" << file;
            }

            break;
        }
        case AboutType::Licenses:
            file.setFileName(directory.absoluteFilePath("README.md"));
            break;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(lcResources()) << "Cannot open file" << file;
    }

    QTextStream in(&file);
    QString     ret = in.readAll();
    file.close();

    emit aboutInfo(ret, directory.absolutePath());
}

QString Resources::getLinkContent(const QString& baseDir, const QString& path) {
    QFile   file;
    QString dir = baseDir;
    file.setFileName(dir.replace("file:/", "") + path);

    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(lcResources()) << "Cannot open file" << file;
        return QString();
    }

    QTextStream in(&file);
    QString     ret = in.readAll();
    file.close();

    return ret;
}

QStringList Resources::getIconList() {
    QStringList list;

    foreach(const QString& key, m_iconList.keys()) {
        list.append("uc:" + key);
    }

    return list;
}

QStringList Resources::getCustomIconList() {
    QDir        dir(m_resourcePaths.value(Icon));
    QStringList files = dir.entryList(QStringList() << "*.png"
                                                    << "*.jpg"
                                                    << "*.jpeg",
                                      QDir::Files | QDir::NoDot | QDir::NoDotAndDotDot);

    for (QString& file : files) {
        file.prepend("custom:");
    }

    return files;
}

QString Resources::getResource(ResourceType type, const QString& id) {
    QString prefix;
    QString resourceName;

    if (id.contains(":")) {
        prefix = id.split(":")[0];
        resourceName = id.split(":")[1];
    }

    qCDebug(lcResources()) << "Prefix:" << prefix << "Name:" << resourceName;

    switch (type) {
        case Icon: {
            // UC icon
            if (prefix.contains("uc")) {
                if (m_iconList.contains(resourceName)) {
                    return m_iconList.value(resourceName).toString();
                } else {
                    qCDebug(lcResources()) << "Cannot find icon:" << id;
                }
            } else if (prefix.contains("custom")) {
                // Custom icon
                if (QFile::exists(m_resourcePaths.value(type) + resourceName)) {
                    return QString("file:" + m_resourcePaths.value(type) + resourceName);
                }

                qCDebug(lcResources()) << "Cannot find custom icon:" << id;
                return QString();
            } else {
                return QString();
            }
        }
        case TvChannelIcon:
        case BackgroundImage:
        case Sound: {
            if (QFile::exists(m_resourcePaths.value(type) + resourceName) && !resourceName.isEmpty()) {
                return QString("file:" + m_resourcePaths.value(type) + resourceName);
            }

            qCDebug(lcResources()) << "Cannot find resource:" << type << id;
            return QString();
        }
    }
}
}  // namespace ui
}  // namespace uc
