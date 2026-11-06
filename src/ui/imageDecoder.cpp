// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "imageDecoder.h"

#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>

namespace uc {
namespace ui {

namespace {

QImage fail(QString* error, const QString& reason) {
    if (error) {
        *error = reason;
    }
    return QImage();
}

/// Fits size into a square box of maxDimension, keeping the aspect ratio and at least one pixel per side.
QSize fitInBox(const QSize& size, int maxDimension) {
    return size.scaled(maxDimension, maxDimension, Qt::KeepAspectRatio).expandedTo(QSize(1, 1));
}

}  // namespace

bool ImageDecoder::isGzip(const QByteArray& data) {
    return data.size() >= 2 && static_cast<quint8>(data.at(0)) == 0x1f && static_cast<quint8>(data.at(1)) == 0x8b;
}

bool ImageDecoder::isSvgFileName(const QString& fileName) {
    return fileName.endsWith(".svg", Qt::CaseInsensitive) || fileName.endsWith(".svgz", Qt::CaseInsensitive);
}

QImage ImageDecoder::decode(const QByteArray& data, int maxDimension, QString* error) {
    if (data.isEmpty()) {
        return fail(error, "no image data");
    }

    if (maxDimension <= 0) {
        return fail(error, "invalid maximum dimension");
    }

    // The SVG plugin inflates gzip data without a limit; no other supported format is gzip.
    if (isGzip(data)) {
        return fail(error, "compressed image data is not supported");
    }

    QBuffer buffer;
    buffer.setData(data);
    buffer.open(QIODevice::ReadOnly);

    QImageReader     reader(&buffer);
    const QByteArray format = reader.format();

    if (format.isEmpty()) {
        return fail(error, "unknown image format");
    }

    const bool scalable = format == "svg" || format == "svgz";

    if (scalable && data.size() > MAX_SVG_BYTES) {
        return fail(error, QString("SVG larger than %1 bytes").arg(MAX_SVG_BYTES));
    }

    // Reading the size costs a header parse for a bitmap and a document parse for an SVG.
    const QSize size = reader.size();

    if (scalable) {
        if (!size.isValid() || size.isEmpty()) {
            return fail(error, "SVG without a size");
        }
        // an SVG has no pixels of its own: it is always rasterized to fit the box
        reader.setScaledSize(fitInBox(size, maxDimension));
    } else if (size.isValid() && !size.isEmpty()) {
        if (static_cast<qint64>(size.width()) * size.height() > MAX_BITMAP_PIXELS) {
            return fail(error, QString("bitmap of %1 x %2 pixels exceeds the limit of %3 pixels")
                                   .arg(size.width())
                                   .arg(size.height())
                                   .arg(MAX_BITMAP_PIXELS));
        }
        if (size.width() > maxDimension || size.height() > maxDimension) {
            reader.setScaledSize(fitInBox(size, maxDimension));
        }
    }

    QImage image = reader.read();

    if (image.isNull()) {
        return fail(error,
                    reader.errorString().isEmpty() ? QString("image could not be decoded") : reader.errorString());
    }

    // a format that does not report its size up front is scaled after decoding
    if (image.width() > maxDimension || image.height() > maxDimension) {
        image = image.scaled(maxDimension, maxDimension, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    return image;
}

bool ImageDecoder::checkSvgFile(const QString& path, QString* error) {
    const QFileInfo info(path);

    if (!info.isFile()) {
        if (error) {
            *error = "file not found";
        }
        return false;
    }

    if (info.size() > MAX_SVG_BYTES) {
        if (error) {
            *error = QString("SVG larger than %1 bytes").arg(MAX_SVG_BYTES);
        }
        return false;
    }

    QFile file(path);

    if (!file.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = "file cannot be read";
        }
        return false;
    }

    if (isGzip(file.peek(2))) {
        if (error) {
            *error = "compressed SVG is not supported";
        }
        return false;
    }

    return true;
}

}  // namespace ui
}  // namespace uc
