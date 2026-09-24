// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>

namespace uc {
namespace ui {

/**
 * @brief The icon font that was loaded: the family it registered and the file it came from.
 */
struct IconFontResult {
    QString family;
    QString path;
};

/**
 * @brief Selects and loads the icon font.
 *
 * The binary embeds the redistributable Free edition of the icon font. The firmware installs the
 * licensed Pro edition as a file and names it in `UC_ICON_FONT_PATH`; when that file exists and
 * loads, it is used instead of the embedded font. Which icons the loaded font can draw is decided
 * per icon by Resources, so nothing else depends on the edition (docs/icon-font.md).
 */
class IconFont {
 public:
    /// The embedded icon font resource.
    static const QString embeddedPath;

    /**
     * @brief Returns the path of the icon font to load: overridePath when it names a readable file,
     * otherwise the embedded font.
     */
    static QString select(const QString& overridePath);

    /**
     * @brief Loads the font in overridePath, or the embedded font when overridePath is empty, does
     * not name a readable file or cannot be loaded. Returns the registered family and the path that
     * was loaded; the family is empty if neither font could be loaded.
     */
    static IconFontResult load(const QString& overridePath);

 private:
    static QString loadFile(const QString& path);
};

}  // namespace ui
}  // namespace uc
