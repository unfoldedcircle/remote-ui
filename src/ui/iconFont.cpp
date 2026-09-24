// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "iconFont.h"

#include <QFileInfo>
#include <QFontDatabase>

#include "../logging.h"

namespace uc {
namespace ui {

const QString IconFont::embeddedPath = QStringLiteral(":icon-font.ttf");

QString IconFont::select(const QString& overridePath) {
    if (overridePath.isEmpty()) {
        return embeddedPath;
    }

    const QFileInfo info(overridePath);
    if (!info.isFile() || !info.isReadable()) {
        qCWarning(lcUi()) << "Icon font override is not a readable file, using the embedded font:" << overridePath;
        return embeddedPath;
    }

    return overridePath;
}

IconFontResult IconFont::load(const QString& overridePath) {
    const QString path = select(overridePath);
    const QString family = loadFile(path);

    if (family.isEmpty() && path != embeddedPath) {
        qCWarning(lcUi()) << "Icon font override could not be loaded, using the embedded font:" << path;
        return {loadFile(embeddedPath), embeddedPath};
    }

    return {family, path};
}

QString IconFont::loadFile(const QString& path) {
    const int id = QFontDatabase::addApplicationFont(path);

    if (id == -1) {
        qCWarning(lcUi()) << "Failed to load font" << path;
        return QString();
    }

    // The family is read from the font instead of being hard coded: it is what the QML binds to.
    const QStringList families = QFontDatabase::applicationFontFamilies(id);

    if (families.isEmpty()) {
        qCWarning(lcUi()) << "Font without a family name" << path;
        return QString();
    }

    return families.first();
}

}  // namespace ui
}  // namespace uc
