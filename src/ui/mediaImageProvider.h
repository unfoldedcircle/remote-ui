// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QHash>
#include <QImage>
#include <QMutex>
#include <QQuickImageProvider>
#include <QQmlEngine>
#include <QStringList>

namespace uc {
namespace ui {

class MediaImageProvider : public QQuickImageProvider {
 public:
    MediaImageProvider();
    ~MediaImageProvider() override;

    static void                install(QQmlEngine* engine);
    static MediaImageProvider* instance();
    static QString             imageUrlForKey(const QString& key);

    QString storeImage(const QString& entityId, quint64 requestId, const QImage& image);
    void    removeImage(const QString& key);

    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;

 private:
    static QString cacheKeyFor(const QString& entityId, quint64 requestId);
    void           touchKeyLocked(const QString& key);
    void           pruneCacheLocked();

    static MediaImageProvider* s_instance;

    mutable QMutex         m_mutex;
    QHash<QString, QImage> m_images;
    // least recently used first: storing and reading an image moves its key to the back
    QStringList m_order;
    // The cache is bounded by memory, not by a number of images: an artwork is up to 1024x1024 (4 MiB), typical
    // artwork a quarter of that, and a remote has dozens of media players. 12 images of any size used to be the
    // limit, which lost the artwork of the 13th player while its tile still pointed at the cache.
    qint64 m_maxBytes = 48 * 1024 * 1024;
    qint64 m_totalBytes = 0;
};

}  // namespace ui
}  // namespace uc
