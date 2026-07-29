// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mediaPlayer.h"

#include <QSslConfiguration>
#include <functional>
#include <utility>

#include "../../logging.h"
#include "../../util.h"
#include "../mediaImageProvider.h"

namespace uc {
namespace ui {
namespace entity {

static constexpr int IMAGE_REQUEST_TIMEOUT_MS = 15000;
static constexpr int MAX_MEDIA_IMAGE_DIMENSION = 1024;

namespace {

struct ProcessedMediaImage {
    QImage  mediaImage;
    QColor  mediaImageColor;
    bool    success = false;
    bool    cancelled = false;
};

QImage normalizeMediaImage(QImage image) {
    if (image.isNull()) {
        return image;
    }

    const int maxDimension = qMax(image.width(), image.height());
    if (maxDimension <= MAX_MEDIA_IMAGE_DIMENSION) {
        return image;
    }

    return image.scaled(MAX_MEDIA_IMAGE_DIMENSION, MAX_MEDIA_IMAGE_DIMENSION, Qt::KeepAspectRatio,
                        Qt::SmoothTransformation);
}

QColor computeAverageImageColorForImage(const QImage &image,
                                        const std::function<bool()> &isRequestCurrent = std::function<bool()>()) {
    if (image.isNull()) {
        return QColor();
    }

    int    step = 20;
    int    t = 0;
    int    r = 0, g = 0, b = 0;
    double brightness = 0.6;

    for (int i = 0; i < image.width(); i += step) {
        if (isRequestCurrent && !isRequestCurrent()) {
            return QColor();
        }

        for (int j = 0; j < image.height(); j += step) {
            if (image.valid(i, j)) {
                t++;
                QColor c = image.pixel(i, j);
                r += c.red();
                b += c.blue();
                g += c.green();
            }
        }
    }

    if (t == 0) {
        return QColor();
    }

    QColor color = QColor(static_cast<int>(brightness * r / t) > 255 ? 255 : static_cast<int>(brightness * r / t),
                          static_cast<int>(brightness * g / t) > 255 ? 255 : static_cast<int>(brightness * g / t),
                          static_cast<int>(brightness * b / t) > 255 ? 255 : static_cast<int>(brightness * b / t));

    if (color.lightness() < 30) {
        color.setHsl(color.hslHue(), color.hslSaturation(), 30);
    }

    return color;
}

ProcessedMediaImage processDownloadedMediaImage(const QByteArray &imageData,
                                                const std::function<bool()> &isRequestCurrent) {
    ProcessedMediaImage result;

    QImage image;
    if (!image.loadFromData(imageData)) {
        return result;
    }

    image = normalizeMediaImage(std::move(image));

    if (isRequestCurrent && !isRequestCurrent()) {
        result.cancelled = true;
        return result;
    }

    result.mediaImage = image;
    result.mediaImageColor = computeAverageImageColorForImage(image, isRequestCurrent);

    if (isRequestCurrent && !isRequestCurrent()) {
        result.cancelled = true;
        return result;
    }

    if (!result.mediaImageColor.isValid()) {
        result.mediaImageColor = QColor("#171717");
    }

    result.success = true;
    return result;
}

ProcessedMediaImage processEmbeddedMediaImage(const QString &dataUrl, const QColor &fallbackColor,
                                              const std::function<bool()> &isRequestCurrent) {
    ProcessedMediaImage result;
    result.mediaImageColor = fallbackColor;

    const QString base64Data = dataUrl.section(",", 1);
    const QByteArray imageData = QByteArray::fromBase64(base64Data.toLatin1());

    QImage image;
    if (!image.loadFromData(imageData)) {
        return result;
    }

    image = normalizeMediaImage(std::move(image));

    if (isRequestCurrent && !isRequestCurrent()) {
        result.cancelled = true;
        return result;
    }

    result.mediaImage = image;
    result.mediaImageColor = computeAverageImageColorForImage(image, isRequestCurrent);

    if (isRequestCurrent && !isRequestCurrent()) {
        result.cancelled = true;
        return result;
    }

    if (!result.mediaImageColor.isValid()) {
        result.mediaImageColor = fallbackColor;
    }

    result.success = true;
    return result;
}

}  // namespace

MediaPlayer::MediaPlayer(const QString &id, QVariantMap nameI18n, const QString &language, const QString &icon,
                         const QString &area, const QString &deviceClass, const QStringList &features, bool enabled,
                         QVariantMap attributes, QVariantMap options, const QString &integrationId, QObject *parent)
    : Base(id, nameI18n, language, icon, area, Type::Media_player, enabled, attributes, integrationId, false, parent),
      m_volume(0),
      m_muted(false),
      m_mediaDuration(0),
      m_mediaPosition(0),
      m_mediaImageColor(QColor("#171717")),
      m_shuffle(false),
      m_repeat(MediaPlayerRepeatMode::Enum::OFF),
      m_volumeSteps(100) {
    qCDebug(lcMediaPlayer()) << "MediaPlayer entity constructor" << id;

    updateFeatures<MediaPlayerFeatures::Enum>(features);

            // attributes
    if (attributes.size() > 0) {
        for (QVariantMap::iterator i = attributes.begin(); i != attributes.end(); i++) {
            updateAttribute(uc::Util::FirstToUpper(i.key()), i.value());
        }
    }

            // device class
    int deviceClassEnum = -1;

    if (!deviceClass.isEmpty()) {
        deviceClassEnum = Util::convertStringToEnum<MediaPlayerDeviceClass::Enum>(deviceClass);
    }

    if (deviceClassEnum != -1) {
        m_deviceClass = deviceClass;
    } else {
        m_deviceClass = QVariant::fromValue(MediaPlayerDeviceClass::Speaker).toString();
    }

            // options
    if (options.contains("volume_steps")) {
        m_volumeSteps = options.value("volume_steps").toInt();
    }

    if (options.contains("simple_commands")) {
        m_simpleCommands = options.value("simple_commands").toStringList();
    }

            // setup position timer
    m_positionTimer.setInterval(1000);
    m_positionTimer.setTimerType(Qt::VeryCoarseTimer);

    QObject::connect(&m_positionTimer, &QTimer::timeout, this, &MediaPlayer::onPositionTimerTimeout);
    QObject::connect(&m_nam, &QNetworkAccessManager::finished, this, &MediaPlayer::onNetworkRequestFinished);

    QObject::connect(&m_nam, QOverload<QNetworkReply *, const QList<QSslError> &>::of(&QNetworkAccessManager::sslErrors),
            this, &MediaPlayer::onSslErrors);
}

MediaPlayer::~MediaPlayer() {
    if (!m_mediaImageCacheKey.isEmpty()) {
        if (auto *provider = MediaImageProvider::instance()) {
            provider->removeImage(m_mediaImageCacheKey);
        }
    }

    qCDebug(lcMediaPlayer()) << "MediaPlayer entity destructor";
}

void MediaPlayer::turnOn() {
    if (hasFeature(MediaPlayerFeatures::On_off)) {
        sendCommand(MediaPlayerCommands::On);
    }
}

void MediaPlayer::turnOff() {
    if (hasFeature(MediaPlayerFeatures::On_off)) {
        sendCommand(MediaPlayerCommands::Off);
    }
}

void MediaPlayer::toggle() {
    if (hasFeature(MediaPlayerFeatures::Toggle)) {
        sendCommand(MediaPlayerCommands::Toggle);
    } else {
        if (m_state == MediaPlayerStates::Off) {
            sendCommand(MediaPlayerCommands::On);
        } else {
            sendCommand(MediaPlayerCommands::Off);
        }
    }
}

void MediaPlayer::playPause() {
    sendCommand(MediaPlayerCommands::Play_pause);
}

void MediaPlayer::stop() {
    sendCommand(MediaPlayerCommands::Stop);
}

void MediaPlayer::previous() {
    sendCommand(MediaPlayerCommands::Previous);
}

void MediaPlayer::next() {
    sendCommand(MediaPlayerCommands::Next);
}

void MediaPlayer::fastForward() {
    sendCommand(MediaPlayerCommands::Fast_forward);
}

void MediaPlayer::rewind() {
    sendCommand(MediaPlayerCommands::Rewind);
}

void MediaPlayer::seek(int mediaPosition) {
    QVariantMap params;
    params.insert("media_position", mediaPosition);
    sendCommand(MediaPlayerCommands::Seek, params);
}

void MediaPlayer::setVolume(int volume) {
    QVariantMap params;
    params.insert("volume", volume);
    sendCommand(MediaPlayerCommands::Volume, params);
}

void MediaPlayer::volumeUp() {
    sendCommand(MediaPlayerCommands::Volume_up);
}

void MediaPlayer::volumeDown() {
    sendCommand(MediaPlayerCommands::Volume_down);
}

void MediaPlayer::muteToggle() {
    sendCommand(MediaPlayerCommands::Mute_toggle);
}

void MediaPlayer::mute() {
    sendCommand(MediaPlayerCommands::Mute);
}

void MediaPlayer::unmute() {
    sendCommand(MediaPlayerCommands::Unmute);
}

void MediaPlayer::repeat() {
    MediaPlayerRepeatMode::Enum newRepeatMode;

    switch (m_repeat) {
        case MediaPlayerRepeatMode::Enum::OFF:
            newRepeatMode = MediaPlayerRepeatMode::Enum::ONE;
            break;
        case MediaPlayerRepeatMode::Enum::ONE:
            newRepeatMode = MediaPlayerRepeatMode::Enum::ALL;
            break;
        case MediaPlayerRepeatMode::Enum::ALL:
            newRepeatMode = MediaPlayerRepeatMode::Enum::OFF;
            break;
    }

    QVariantMap params;
    params.insert("repeat", Util::convertEnumToString(newRepeatMode));
    sendCommand(MediaPlayerCommands::Repeat, params);
}

void MediaPlayer::shuffle() {
    QVariantMap params;
    params.insert("shuffle", !m_shuffle);
    sendCommand(MediaPlayerCommands::Shuffle, params);
}

void MediaPlayer::channelUp() {
    sendCommand(MediaPlayerCommands::Channel_up);
}

void MediaPlayer::channelDown() {
    sendCommand(MediaPlayerCommands::Channel_down);
}

void MediaPlayer::cursorUp() {
    sendCommand(MediaPlayerCommands::Cursor_up);
}

void MediaPlayer::cursorDown() {
    sendCommand(MediaPlayerCommands::Cursor_down);
}

void MediaPlayer::cursorLeft() {
    sendCommand(MediaPlayerCommands::Cursor_left);
}

void MediaPlayer::cursorRight() {
    sendCommand(MediaPlayerCommands::Cursor_right);
}

void MediaPlayer::cursorEnter() {
    sendCommand(MediaPlayerCommands::Cursor_enter);
}

void MediaPlayer::digit0()
{
    sendCommand(MediaPlayerCommands::Digit_0);
}

void MediaPlayer::digit1()
{
    sendCommand(MediaPlayerCommands::Digit_1);
}

void MediaPlayer::digit2()
{
    sendCommand(MediaPlayerCommands::Digit_2);
}

void MediaPlayer::digit3()
{
    sendCommand(MediaPlayerCommands::Digit_3);
}

void MediaPlayer::digit4()
{
    sendCommand(MediaPlayerCommands::Digit_4);
}

void MediaPlayer::digit5()
{
    sendCommand(MediaPlayerCommands::Digit_5);
}

void MediaPlayer::digit6()
{
    sendCommand(MediaPlayerCommands::Digit_6);
}

void MediaPlayer::digit7()
{
    sendCommand(MediaPlayerCommands::Digit_7);
}

void MediaPlayer::digit8()
{
    sendCommand(MediaPlayerCommands::Digit_8);
}

void MediaPlayer::digit9()
{
    sendCommand(MediaPlayerCommands::Digit_9);
}

void MediaPlayer::functionRed() {
    sendCommand(MediaPlayerCommands::Function_red);
}

void MediaPlayer::functionGreen() {
    sendCommand(MediaPlayerCommands::Function_green);
}

void MediaPlayer::functionYellow() {
    sendCommand(MediaPlayerCommands::Function_yellow);
}

void MediaPlayer::functionBlue() {
    sendCommand(MediaPlayerCommands::Function_blue);
}

void MediaPlayer::home() {
    sendCommand(MediaPlayerCommands::Home);
}

void MediaPlayer::menu() {
    sendCommand(MediaPlayerCommands::Menu);
}

void MediaPlayer::contextMenu()
{
    sendCommand(MediaPlayerCommands::Context_menu);
}

void MediaPlayer::guide()
{
    sendCommand(MediaPlayerCommands::Guide);
}

void MediaPlayer::info()
{
    sendCommand(MediaPlayerCommands::Info);
}

void MediaPlayer::back() {
    sendCommand(MediaPlayerCommands::Back);
}

void MediaPlayer::selectSource(const QString &source) {
    QVariantMap params;
    params.insert("source", source);
    sendCommand(MediaPlayerCommands::Select_source, params);
}

void MediaPlayer::record()
{
    sendCommand(MediaPlayerCommands::Record);
}

void MediaPlayer::myRecordings()
{
    sendCommand(MediaPlayerCommands::My_recordings);
}

void MediaPlayer::live()
{
    sendCommand(MediaPlayerCommands::Live);
}

void MediaPlayer::eject()
{
    sendCommand(MediaPlayerCommands::Eject);
}

void MediaPlayer::openClose()
{
    sendCommand(MediaPlayerCommands::Open_close);
}

void MediaPlayer::audioTrack()
{
    sendCommand(MediaPlayerCommands::Audio_track);
}

void MediaPlayer::subtitle()
{
    sendCommand(MediaPlayerCommands::Subtitle);
}

void MediaPlayer::settings()
{
    sendCommand(MediaPlayerCommands::Settings);
}

void MediaPlayer::playMedia(const QString &mediaId, const QString &mediaType, const QString &action) {
    QVariantMap params;
    params.insert("media_id", mediaId);
    params.insert("media_type", mediaType);
    if (!action.isEmpty()) {
        params.insert("action", action);
    }
    sendCommand(MediaPlayerCommands::Play_media, params);
}

void MediaPlayer::clearPlaylist() {
    sendCommand(MediaPlayerCommands::Clear_playlist);
}

void MediaPlayer::browseMedia(const QString &mediaId, const QString &mediaType, int limit, int page) {
    QVariantMap params;
    if (!mediaId.isEmpty()) {
        params.insert("media_id", mediaId);
    }
    if (!mediaType.isEmpty()) {
        params.insert("media_type", mediaType);
    }
    QVariantMap paging;
    paging.insert("limit", limit);
    paging.insert("page", page);
    params.insert("paging", paging);
    emit browseMediaRequested(m_id, params);
}

void MediaPlayer::searchMedia(const QString &query, const QString &mediaId, const QString &mediaType,
                               const QStringList &mediaClasses, int limit, int page) {
    QVariantMap params;
    params.insert("query", query);
    if (!mediaId.isEmpty()) {
        params.insert("media_id", mediaId);
    }
    if (!mediaType.isEmpty()) {
        params.insert("media_type", mediaType);
    }
    if (!mediaClasses.isEmpty()) {
        QVariantMap filter;
        filter.insert("media_classes", mediaClasses);
        params.insert("filter", filter);
    }
    QVariantMap paging;
    paging.insert("limit", limit);
    paging.insert("page", page);
    params.insert("paging", paging);
    emit searchMediaRequested(m_id, params);
}

void MediaPlayer::onBrowseMediaResult(const core::BrowseMediaItem &media, const core::Pagination &pagination) {
    emit browseMediaResult(browseItemToVariant(media), paginationToVariant(pagination));
}

void MediaPlayer::onSearchMediaResult(const QList<core::BrowseMediaItem> &items, const core::Pagination &pagination) {
    QVariantList list;
    for (const auto &item : items) {
        list.append(browseItemToVariant(item));
    }
    emit searchMediaResult(list, paginationToVariant(pagination));
}

void MediaPlayer::onMediaBrowseError(int code, const QString &message) {
    emit mediaBrowseError(code, message);
}

QVariantMap MediaPlayer::browseItemToVariant(const core::BrowseMediaItem &item) {
    QVariantMap map;
    map.insert("media_id", item.mediaId);
    map.insert("title", item.title);
    map.insert("artist", item.artist);
    map.insert("album", item.album);
    map.insert("media_class", item.mediaClass);
    map.insert("media_type", item.mediaType);
    map.insert("can_browse", item.canBrowse);
    map.insert("can_play", item.canPlay);
    map.insert("can_search", item.canSearch);
    map.insert("thumbnail", item.thumbnail);
    map.insert("duration", item.duration);
    map.insert("subtitle", item.subtitle);
    map.insert("play_media_action", item.playMediaActions);

    QVariantList children;
    for (const auto &child : item.items) {
        children.append(browseItemToVariant(child));
    }
    map.insert("items", children);

    return map;
}

QVariantMap MediaPlayer::paginationToVariant(const core::Pagination &p) {
    QVariantMap map;
    map.insert("count", p.count);
    map.insert("limit", p.limit);
    map.insert("page", p.page);
    return map;
}

void MediaPlayer::getMediaImageColor(QString imageUrl, quint64 requestId) {
    if (imageUrl.isEmpty()) {
        clearMediaImageState();
        return;
    }

    QNetworkRequest request(imageUrl);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);
    // weird API: no default timeout set in QNetworkRequest, calling setTransferTimeout without parameter sets it to 30s
    request.setTransferTimeout(IMAGE_REQUEST_TIMEOUT_MS);

    // Create SSL configuration that ignores certificate errors
    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);

    request.setSslConfiguration(sslConfig);

    // note: not logging url because it might contain sensitive information like API keys (e.g. Plex)
    qCInfo(lcMediaPlayer()) << "Starting image download with timeout:" << IMAGE_REQUEST_TIMEOUT_MS;
    QNetworkReply *reply = m_nam.get(request);
    reply->setProperty("mediaImageUrl", imageUrl);
    reply->setProperty("mediaImageRequestId", QVariant::fromValue<qulonglong>(requestId));

    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
            this, &MediaPlayer::onNetworkError);
}

void MediaPlayer::clearMediaImageState() {
    ++m_mediaImageProcessingRequestId;

    if (!m_mediaImageCacheKey.isEmpty()) {
        if (auto *provider = MediaImageProvider::instance()) {
            provider->removeImage(m_mediaImageCacheKey);
        }
        m_mediaImageCacheKey.clear();
    }

    if (!m_mediaImage.isEmpty()) {
        m_mediaImage.clear();
        emit mediaImageChanged();
    }

    const QColor defaultColor("#171717");
    if (m_mediaImageColor != defaultColor) {
        m_mediaImageColor = defaultColor;
        emit mediaImageColorChanged();
    }
}

void MediaPlayer::processMediaImageAsync(const QString &imageUrl, const QByteArray &imageData, quint64 requestId,
                                         const QString &dataUrl) {
    const QColor fallbackColor = QColor("#171717");
    QPointer<MediaPlayer> guard(this);

    QThreadPool::globalInstance()->start([guard, requestId, imageUrl, imageData, dataUrl, fallbackColor]() {
        if (!guard) {
            return;
        }

        const auto isRequestCurrent = [guard, requestId]() {
            return guard && guard->isMediaImageRequestCurrent(requestId);
        };

        ProcessedMediaImage result;
        if (dataUrl.isEmpty()) {
            result = processDownloadedMediaImage(imageData, isRequestCurrent);
        } else {
            result = processEmbeddedMediaImage(dataUrl, fallbackColor, isRequestCurrent);
        }

        if (result.cancelled) {
            return;
        }

        if (!guard) {
            return;
        }

        QMetaObject::invokeMethod(
            guard,
            [guard, imageUrl, requestId, result]() {
                if (!guard) {
                    return;
                }

                guard->applyProcessedMediaImage(imageUrl, requestId, result.mediaImage, result.mediaImageColor,
                                                result.success);
            },
            Qt::QueuedConnection);
    });
}

void MediaPlayer::applyProcessedMediaImage(const QString &imageUrl, quint64 requestId, const QImage &mediaImage,
                                           const QColor &mediaImageColor, bool success) {
    if (requestId != m_mediaImageProcessingRequestId.load() || imageUrl != m_mediaImageUrl) {
        qCDebug(lcMediaPlayer()) << "Ignoring stale processed image result";
        return;
    }

    if (!success || mediaImage.isNull()) {
        clearMediaImageState();
        return;
    }

    auto *provider = MediaImageProvider::instance();
    if (!provider) {
        qCWarning(lcMediaPlayer()) << "Media image provider is not available";
        clearMediaImageState();
        return;
    }

    const QString previousCacheKey = m_mediaImageCacheKey;
    const QString nextCacheKey = provider->storeImage(m_id, requestId, mediaImage);
    const QString nextMediaImage = MediaImageProvider::imageUrlForKey(nextCacheKey);

    if (m_mediaImage != nextMediaImage) {
        m_mediaImage = nextMediaImage;
        emit mediaImageChanged();
    }

    m_mediaImageCacheKey = nextCacheKey;

    if (!previousCacheKey.isEmpty() && previousCacheKey != nextCacheKey) {
        provider->removeImage(previousCacheKey);
    }

    if (m_mediaImageColor != mediaImageColor) {
        m_mediaImageColor = mediaImageColor;
        qCDebug(lcMediaPlayer()).noquote() << "Background image lightness" << m_mediaImageColor.lightness();
        emit mediaImageColorChanged();
    }
}

bool MediaPlayer::isMediaImageRequestCurrent(quint64 requestId) const {
    return m_mediaImageProcessingRequestId.load() == requestId;
}

void MediaPlayer::sendCommand(MediaPlayerCommands::Enum cmd, QVariantMap params) {
    Base::sendCommand(QVariant::fromValue(cmd).toString(), params);
}

void MediaPlayer::sendCommand(MediaPlayerCommands::Enum cmd) {
    sendCommand(cmd, QVariantMap());
}

void MediaPlayer::sendSimpleCommand(QString command)
{
    if (!m_simpleCommands.contains(command)) {
        qCWarning(lcMediaPlayer()) << "Simple command is not supported" << command;
        return;
    }

    Base::sendCommand(command);
}

bool MediaPlayer::updateAttribute(const QString &attribute, QVariant data) {
    bool ok = false;

            // convert to enum
    MediaPlayerAttributes::Enum attributeEnum = Util::convertStringToEnum<MediaPlayerAttributes::Enum>(attribute);

    switch (attributeEnum) {
        case MediaPlayerAttributes::State: {
            bool stateOk = false;
            int newState = Util::convertStringToEnum<MediaPlayerStates::Enum>(uc::Util::FirstToUpper(data.toString()), &stateOk);
            const int nextState = stateOk ? newState : MediaPlayerStates::Unknown;

            ok = true;
            if (m_state == nextState) {
                break;
            }

            m_state = nextState;
            emit stateChanged(m_id, m_state);

            m_stateAsString = MediaPlayerStates::getTranslatedString(static_cast<MediaPlayerStates::Enum>(m_state));
            emit stateAsStringChanged();

             // enable/disable media position timer
            if (m_state == MediaPlayerStates::Playing) {
                m_positionTimer.start();
                emit addToActivities(m_id);
            } else {
                m_positionTimer.stop();
            }

            if (m_state == MediaPlayerStates::Off) {
                ++m_mediaImageProcessingRequestId;

                m_mediaDuration = 0;
                emit mediaDurationChanged();

                m_mediaPosition = 0;
                emit mediaPositionChanged();

                m_mediaImageUrl.clear();
                emit mediaImageUrlChanged();

                if (!m_mediaImageCacheKey.isEmpty()) {
                    if (auto *provider = MediaImageProvider::instance()) {
                        provider->removeImage(m_mediaImageCacheKey);
                    }
                    m_mediaImageCacheKey.clear();
                }

                m_mediaImage = QString();
                emit mediaImageChanged();
                m_mediaImageColor = QColor(255,255,255);
                emit mediaImageColorChanged();

                        //                getMediaImageColor(m_mediaImageUrl);

                m_mediaTitle.clear();
                emit mediaTitleChanged();
                emit stateInfoChanged();

                m_mediaAlbum.clear();
                emit mediaAlbumChanged();

                m_mediaArtist.clear();
                emit mediaArtistChanged();

                m_mediaType = "";
                emit mediaTypeChanged();

                emit removeFromActivities(m_id);
            }
            break;
        }
        case MediaPlayerAttributes::Volume: {
            int newVolume = data.toInt();

            if (m_volume != newVolume) {
                m_volume = newVolume;
                ok = true;
                emit volumeChanged();
            }
            break;
        }
        case MediaPlayerAttributes::Muted: {
            int newVal = data.toBool();

            if (m_muted != newVal) {
                m_muted = newVal;
                ok = true;
                emit mutedChanged();
            }
            break;
        }
        case MediaPlayerAttributes::Media_duration: {
            int newDuration = data.toInt();

            if (m_mediaDuration != newDuration) {
                m_mediaDuration = newDuration;
                ok = true;
                emit mediaDurationChanged();
            }
            break;
        }
        case MediaPlayerAttributes::Media_position: {
            int newPosition = data.toInt();

            if (m_mediaPosition != newPosition) {
                m_mediaPosition = newPosition;
                ok = true;
                emit mediaPositionChanged();
            }
            break;
        }
        case MediaPlayerAttributes::Media_type: {
            QString newType = data.toString();

            m_mediaType = Util::FirstToUpper(newType);
            ok = true;
            emit mediaTypeChanged();
            break;
        }
        case MediaPlayerAttributes::Media_image_url: {
            QString newImageUrl = data.toString();

            if (m_mediaImageUrl != newImageUrl) {
                m_mediaImageUrl = newImageUrl;
                ok = true;
                emit mediaImageUrlChanged();

                m_mediaImageDownloadTries = 0;
                const quint64 requestId = ++m_mediaImageProcessingRequestId;

                bool isBase64 = newImageUrl.startsWith("data:image/", Qt::CaseInsensitive) && newImageUrl.contains(";base64,");

                if (!isBase64) {
                    getMediaImageColor(m_mediaImageUrl, requestId);
                } else {
                    processMediaImageAsync(m_mediaImageUrl, QByteArray(), requestId, newImageUrl);
                }
            }
            break;
        }
        case MediaPlayerAttributes::Media_title: {
            QString newTitle = data.toString();

            m_mediaTitle = newTitle;
            ok = true;
            emit mediaTitleChanged();
            emit stateInfoChanged();
            break;
        }
        case MediaPlayerAttributes::Media_artist: {
            QString newArtist = data.toString();

            m_mediaArtist = newArtist;
            ok = true;
            emit mediaArtistChanged();

            break;
        }
        case MediaPlayerAttributes::Media_album: {
            QString newAlbum = data.toString();

            m_mediaAlbum = newAlbum;
            ok = true;
            emit mediaAlbumChanged();

            break;
        }
        case MediaPlayerAttributes::Shuffle: {
            bool newShuffle = data.toBool();
            if (m_shuffle != newShuffle) {
                m_shuffle = newShuffle;
                ok = true;
                emit shuffleChanged();
            }
            break;
        }
        case MediaPlayerAttributes::Repeat: {
            MediaPlayerRepeatMode::Enum newRepeat =
                Util::convertStringToEnum<MediaPlayerRepeatMode::Enum>(data.toString());
            if (m_repeat != newRepeat) {
                m_repeat = newRepeat;
                ok = true;
                emit repeatChanged();
            }
            break;
        }
        case MediaPlayerAttributes::Source: {
            QString newSource = data.toString();

            m_source = newSource;
            ok = true;
            emit sourceChanged();

            break;
        }
        case MediaPlayerAttributes::Source_list: {
            QStringList newSourceList = data.toStringList();

            m_sourceList = newSourceList;
            ok = true;
            emit sourceListChanged();

            break;
        }
        case MediaPlayerAttributes::Sound_mode:
        case MediaPlayerAttributes::Sound_mode_list:
            // TODO(marton): implement me
            break;
        case MediaPlayerAttributes::Media_id: {
            QString newId = data.toString();
            if (m_mediaId != newId) {
                m_mediaId = newId;
                ok = true;
                emit mediaIdChanged();
            }
            break;
        }
        case MediaPlayerAttributes::Media_playlist: {
            QString newPlaylist = data.toString();
            if (m_mediaPlaylist != newPlaylist) {
                m_mediaPlaylist = newPlaylist;
                ok = true;
                emit mediaPlaylistChanged();
            }
            break;
        }
        case MediaPlayerAttributes::Search_media_classes: {
            QStringList newClasses;
            const QStringList incomingClasses = data.toStringList();

            for (const QString &mediaClass : incomingClasses) {
                bool valid = false;
                Util::convertStringToEnum<MediaClass::Enum>(uc::Util::FirstToUpper(mediaClass), &valid);

                if (valid) {
                    newClasses.append(mediaClass);
                } else {
                    qCWarning(lcMediaPlayer()) << "Ignoring unsupported media class" << mediaClass;
                }
            }

            if (m_searchMediaClasses != newClasses) {
                m_searchMediaClasses = newClasses;
                ok = true;
                emit searchMediaClassesChanged();
            }
            break;
        }
    }

    return ok;
}

void MediaPlayer::onLanguageChangedTypeSpecific()
{
    QTimer::singleShot(500, [=]() {
        m_stateAsString = MediaPlayerStates::getTranslatedString(static_cast<MediaPlayerStates::Enum>(m_state));
        emit stateAsStringChanged();
    });
}

void MediaPlayer::onPositionTimerTimeout() {
    m_mediaPosition++;
    if (m_mediaPosition >= m_mediaDuration) {
        m_mediaPosition = m_mediaDuration;
    }
    emit mediaPositionChanged();
}

void MediaPlayer::onSslErrors(QNetworkReply *reply, const QList<QSslError> &errors) {
    for (const QSslError &error : errors) {
        qCWarning(lcMediaPlayer()) << "Ignorning image download SSL error:" << error;

        // Log certificate info
        QSslCertificate cert = error.certificate();
        if (!cert.isNull()) {
            qCInfo(lcMediaPlayer()) << "Certificate Subject:" << cert.subjectInfo(QSslCertificate::CommonName)
                                    << "Issuer:" << cert.issuerInfo(QSslCertificate::CommonName)
                                    << "Validity:" << cert.effectiveDate().toString(Qt::ISODate)
                                    << "-" << cert.expiryDate().toString(Qt::ISODate);
        }
    }

    // Ignore ALL SSL errors (including hostname mismatch)
    reply->ignoreSslErrors(errors);
}

void MediaPlayer::onNetworkError(QNetworkReply::NetworkError error) {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) {
        return;
    }

    qCWarning(lcMediaPlayer()) << "Image download network error:"
                               << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()
                               << error << reply->errorString();
}

void MediaPlayer::onNetworkRequestFinished(QNetworkReply *reply) {
    const QString replyImageUrl = reply->property("mediaImageUrl").toString();
    const quint64 replyRequestId = reply->property("mediaImageRequestId").toULongLong();

    if (replyImageUrl != m_mediaImageUrl || replyRequestId != m_mediaImageProcessingRequestId.load()) {
        qCDebug(lcMediaPlayer()) << "Ignoring stale image download response";
        reply->deleteLater();
        return;
    }

    if (reply->error()) {
        m_mediaImageDownloadTries++;
        qCWarning(lcMediaPlayer()).nospace() << "Image download failed "
                                             << m_mediaImageDownloadTries << "/3 ("
                                             << reply->error() << " " << reply->errorString() << "): "
                                             << m_mediaImageUrl;

        if (m_mediaImageDownloadTries >= 3) {
            clearMediaImageState();
            reply->deleteLater();
            return;
        } else {
            QTimer::singleShot(1000, this, [this, replyImageUrl, replyRequestId] {
                if (replyImageUrl == m_mediaImageUrl && replyRequestId == m_mediaImageProcessingRequestId.load()) {
                    getMediaImageColor(replyImageUrl, replyRequestId);
                }
            });
        }

        reply->deleteLater();
    } else {
        const QByteArray imageData = reply->readAll();

        qCInfo(lcMediaPlayer()) << "Image successfully downloaded";
        processMediaImageAsync(replyImageUrl, imageData, replyRequestId);

        reply->deleteLater();
    }
}

}  // namespace entity
}  // namespace ui
}  // namespace uc
