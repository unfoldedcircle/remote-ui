// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QByteArray>
#include <QImage>
#include <QSize>
#include <QSizeF>
#include <QString>

namespace uc {
namespace ui {

/**
 * @brief Bounded decoding of images the UI does not control: media artwork, driver-provided images
 *        and custom icon files.
 *
 * The Qt image format plugins decode whatever they are given, at whatever size the data declares.
 * For a bitmap the work is bounded by the file; for an SVG it is not: the declared size decides how
 * large the raster gets, and a gzip-compressed SVG is inflated without a limit. This class keeps
 * the limits in one place so that every decoder in the UI applies the same ones.
 */
class ImageDecoder {
 public:
    /// Largest SVG document the UI decodes, in bytes. Well above the core's upload limit for icons.
    static constexpr qint64 MAX_SVG_BYTES = 256 * 1024;
    /// Largest bitmap the UI decodes, in pixels: 32 megapixels is 128 MB decoded.
    static constexpr qint64 MAX_BITMAP_PIXELS = 32LL * 1024 * 1024;

    /**
     * @brief Decodes image data of any format Qt detects, the longer side at most maxDimension pixels.
     *
     * A bitmap larger than the box is downscaled while decoding, an SVG is rasterized to fit the box.
     * Returns a null image and sets error when the data is rejected: an unknown format, gzip data
     * (`.svgz`), an SVG larger than MAX_SVG_BYTES or without an intrinsic size, or a bitmap above
     * MAX_BITMAP_PIXELS.
     */
    static QImage decode(const QByteArray& data, int maxDimension, QString* error = nullptr);

    /**
     * @brief Checks an SVG file before it is handed to the QML image loader.
     *
     * The image loader rasterizes at the requested size, so only what the file can cost before
     * rasterization is checked: it must exist, be at most MAX_SVG_BYTES and not be gzip-compressed.
     */
    static bool checkSvgFile(const QString& path, QString* error = nullptr);

    /**
     * @brief The largest size with the aspect ratio of source that fits into box, rounded to whole pixels.
     *
     * At least one pixel per side. Returns an invalid size when source or box is empty.
     */
    static QSize fitSize(const QSizeF& source, const QSizeF& box);

    /// True for a `.svg` or `.svgz` file name, matched case-insensitively.
    static bool isSvgFileName(const QString& fileName);

    /// True when the data starts with the gzip magic number.
    static bool isGzip(const QByteArray& data);
};

}  // namespace ui
}  // namespace uc
