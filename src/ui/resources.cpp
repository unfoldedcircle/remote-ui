// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "resources.h"

#include <QFileInfo>
#include <QTextStream>
#include <QUrl>

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

    QFile fallbackFile(":icon-fallback.json");

    if (!fallbackFile.open(QIODevice::ReadOnly)) {
        qCWarning(lcResources()) << "Cannot open icon fallback file";
    } else {
        QJsonObject fallback = QJsonDocument::fromJson(fallbackFile.readAll()).object();

        m_iconFallback = fallback.value("fallback").toObject();
        m_iconPlaceholder = fallback.value("placeholder").toString();

        qCDebug(lcResources()) << "Icon fallback mapping loaded:" << m_iconFallback.count() << "icons";
    }

    qmlRegisterUncreatableType<Resources>("ResourceTypes", 1, 0, "ResourceTypes", "Enum is not a type");
}

Resources::~Resources() {}

void Resources::setIconFont(const QString& family) {
    // The physical font itself: QFontMetrics::inFontUcs4() also answers for Qt's fallback fonts, so a code point
    // that any installed font happens to have would count as drawable and be drawn with that font's glyph.
    m_iconFont = QRawFont::fromFont(QFont(family));

    if (m_iconFont.familyName() != family) {
        qCWarning(lcResources()) << "Icon font family" << family << "resolves to" << m_iconFont.familyName();
    }

    qCDebug(lcResources()) << "Icon font family:" << family;
}

bool Resources::canRenderGlyph(const QString& glyph) const {
    if (glyph.isEmpty()) {
        return false;
    }

    // Without a font the icons are rendered by whatever QML picks: don't second-guess it.
    if (!m_iconFont.isValid()) {
        return true;
    }

    return m_iconFont.supportsCharacter(glyph.toUcs4().first());
}

QString Resources::getIconGlyph(const QString& name) {
    if (!m_iconList.contains(name)) {
        qCDebug(lcResources()) << "Cannot find icon:" << name;
        return QString();
    }

    QString glyph = m_iconList.value(name).toString();

    if (canRenderGlyph(glyph)) {
        return glyph;
    }

    // The embedded font doesn't have this icon: it is only in the Pro edition of the icon set.
    const QString alternative = m_iconFallback.value(name).toString();
    const QString alternativeGlyph = m_iconList.value(alternative).toString();

    if (!alternativeGlyph.isEmpty() && canRenderGlyph(alternativeGlyph)) {
        qCDebug(lcResources()) << "Icon" << name << "not in the icon font, using" << alternative;
        return alternativeGlyph;
    }

    const QString placeholderGlyph = m_iconList.value(m_iconPlaceholder).toString();

    qCDebug(lcResources()) << "Icon" << name << "not in the icon font and without a fallback,"
                           << "using" << m_iconPlaceholder;

    return canRenderGlyph(placeholderGlyph) ? placeholderGlyph : QString();
}

QString Resources::getIcon(const QString& id, const QString& suffix) {
    QString _id;
    QString ext;

    if (id.isEmpty()) {
        qCWarning(lcResources()) << "Empty ID passed to getIcon()";
        return QString();
    }

    if (id.endsWith(".png") || id.endsWith(".jpg")) {
        if (id.length() >= 4) {
            _id = id.chopped(4);
            ext = id.right(4);
        } else {
            qCWarning(lcResources()) << "ID too short for .png/.jpg extension:" << id;
            return QString();  // Or handle it gracefully
        }
    } else if (id.endsWith(".jpeg")) {
        if (id.length() >= 5) {
            _id = id.chopped(5);
            ext = id.right(5);
        } else {
            qCWarning(lcResources()) << "ID too short for .jpeg extension:" << id;
            return QString();
        }
    } else {
        _id = id;
    }

    QString icon;
    ResourceType resourceType = Icon;

            // suffix not supported yet
    //    if (!suffix.isEmpty()) {
    //        icon = getResource(Icon, _id + "-" + suffix.toLower() + ext);
    //    }

    if (_id.contains("ctv:")) {
        resourceType = TvChannelIcon;
    }

    if (icon.isEmpty()) {
        icon = getResource(resourceType, _id + ext);
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

void Resources::getLinkContent(const QString& baseDir, const QString& path) {
    // QML hands in the base URL of its text item: "file:" + directory + "/". Only the prefix is removed, the rest
    // is the directory as it was reported by aboutInfo.
    QString localDir = baseDir;
    if (localDir.startsWith("file:")) {
        localDir.remove(0, 5);
        while (localDir.startsWith("//")) {
            localDir.remove(0, 1);
        }
    }
    const QDir directory(localDir);

    // Only documents of the legal directory are shown: never an external URL, whatever its scheme, and no path
    // that leads out of the directory. The check is lexical, a link inside the directory may be a symbolic link.
    const QString filePath = QDir::cleanPath(directory.absoluteFilePath(path));
    QString       legalRoot = QDir::cleanPath(QDir(m_legalPath + "/").absolutePath());
    if (!legalRoot.endsWith('/')) {
        legalRoot.append('/');
    }
    const bool externalLink = !QUrl(path).isRelative() || path.startsWith("//");

    QString ret;
    QString contentDir = directory.absolutePath();

    if (path.isEmpty() || externalLink || !filePath.startsWith(legalRoot)) {
        qCWarning(lcResources()) << "Not a document of the legal directory, link is not followed:" << path;
    } else {
        QFile file(filePath);

        if (!file.open(QIODevice::ReadOnly)) {
            qCWarning(lcResources()) << "Cannot open file" << filePath;
        } else {
            QTextStream in(&file);
            ret = in.readAll();
            file.close();
            // relative links and images of the document are resolved against its own directory
            contentDir = QFileInfo(filePath).absolutePath();
        }
    }

    emit aboutInfo(ret, contentDir);
}

QStringList Resources::getIconList() {
    QStringList list;

    foreach(const QString& key, m_iconList.keys()) {
        // Don't offer icons the embedded font cannot draw: they would all look like the
        // placeholder in the icon selection.
        if (canRenderGlyph(m_iconList.value(key).toString())) {
            list.append("uc:" + key);
        }
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

    QStringList parts = id.split(":");
    if (parts.size() >= 2) {
        prefix = parts[0];
        resourceName = parts[1];
    } else {
        qCWarning(lcResources()) << "Invalid id format, missing ':' in" << id;
        return QString();  // or some fallback
    }

    // qCDebug(lcResources()) << "Prefix:" << prefix << "Name:" << resourceName;

    switch (type) {
        case Icon: {
            // UC icon
            if (prefix.contains("uc")) {
                return getIconGlyph(resourceName);
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
        case TvChannelIcon: {
            if (QFile::exists(m_resourcePaths.value(type) + resourceName)) {
                return QString("file:" + m_resourcePaths.value(type) + resourceName);
            }

            qCDebug(lcResources()) << "Cannot find TV channel icon:" << id;
            return QString();
        }
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
