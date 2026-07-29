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

    mutable QMutex    m_mutex;
    QHash<QString, QImage> m_images;
    QStringList       m_order;
    int               m_maxEntries = 12;
};

}  // namespace ui
}  // namespace uc
