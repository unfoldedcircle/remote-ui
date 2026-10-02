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
    const auto   previous = m_images.constFind(key);
    if (previous != m_images.cend()) {
        m_totalBytes -= previous->sizeInBytes();
    }
    m_images.insert(key, image);
    m_totalBytes += image.sizeInBytes();
    touchKeyLocked(key);
    pruneCacheLocked();

    return key;
}

void MediaImageProvider::removeImage(const QString& key) {
    if (key.isEmpty()) {
        return;
    }

    QMutexLocker locker(&m_mutex);
    const auto   it = m_images.constFind(key);
    if (it != m_images.cend()) {
        m_totalBytes -= it->sizeInBytes();
        m_images.erase(it);
    }
    m_order.removeAll(key);
}

QImage MediaImageProvider::requestImage(const QString& id, QSize* size, const QSize& requestedSize) {
    QImage image;

    {
        QMutexLocker locker(&m_mutex);
        image = m_images.value(id);
        if (!image.isNull()) {
            // read again: the least recently shown artwork is the first to go
            touchKeyLocked(id);
        }
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
    // the newest image always stays, however large
    while (m_totalBytes > m_maxBytes && m_order.size() > 1) {
        const QString oldestKey = m_order.takeFirst();
        const auto    it = m_images.constFind(oldestKey);
        if (it != m_images.cend()) {
            m_totalBytes -= it->sizeInBytes();
            m_images.erase(it);
        }
    }
}

}  // namespace ui
}  // namespace uc
