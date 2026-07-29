// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mediaImageProvider.h"

#include <QCryptographicHash>
#include <QMutexLocker>

namespace uc {
namespace ui {

MediaImageProvider* MediaImageProvider::s_instance = nullptr;

MediaImageProvider::MediaImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image) {
    s_instance = this;
}

MediaImageProvider::~MediaImageProvider() {
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

void MediaImageProvider::install(QQmlEngine* engine) {
    if (!engine || s_instance) {
        return;
    }

    engine->addImageProvider("media-art", new MediaImageProvider());
}

MediaImageProvider* MediaImageProvider::instance() {
    return s_instance;
}

QString MediaImageProvider::imageUrlForKey(const QString& key) {
    if (key.isEmpty()) {
        return QString();
    }

    return QStringLiteral("image://media-art/%1").arg(key);
}

QString MediaImageProvider::storeImage(const QString& entityId, quint64 requestId, const QImage& image) {
    if (image.isNull()) {
        return QString();
    }

    const QString key = cacheKeyFor(entityId, requestId);

    QMutexLocker locker(&m_mutex);
    m_images.insert(key, image);
    touchKeyLocked(key);
    pruneCacheLocked();

    return key;
}

void MediaImageProvider::removeImage(const QString& key) {
    if (key.isEmpty()) {
        return;
    }

    QMutexLocker locker(&m_mutex);
    m_images.remove(key);
    m_order.removeAll(key);
}

QImage MediaImageProvider::requestImage(const QString& id, QSize* size, const QSize& requestedSize) {
    QImage image;

    {
        QMutexLocker locker(&m_mutex);
        image = m_images.value(id);
    }

    if (size) {
        *size = image.size();
    }

    if (image.isNull() || !requestedSize.isValid() || requestedSize.isEmpty()) {
        return image;
    }

    if (image.width() <= requestedSize.width() && image.height() <= requestedSize.height()) {
        return image;
    }

    return image.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QString MediaImageProvider::cacheKeyFor(const QString& entityId, quint64 requestId) {
    const QByteArray seed = entityId.toUtf8() + ':' + QByteArray::number(requestId);
    return QString::fromLatin1(QCryptographicHash::hash(seed, QCryptographicHash::Sha1).toHex());
}

void MediaImageProvider::touchKeyLocked(const QString& key) {
    m_order.removeAll(key);
    m_order.append(key);
}

void MediaImageProvider::pruneCacheLocked() {
    while (m_order.size() > m_maxEntries) {
        const QString oldestKey = m_order.takeFirst();
        m_images.remove(oldestKey);
    }
}

}  // namespace ui
}  // namespace uc
