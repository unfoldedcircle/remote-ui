// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mediaPlayer.h"

#include "../../logging.h"
#include "../../util.h"

namespace uc {
namespace ui {
namespace entity {

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
}

MediaPlayer::~MediaPlayer() {
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

void MediaPlayer::getMediaImageColor(QString imageUrl) {
    if (imageUrl.isEmpty()) {
        m_mediaImage = QString();
        emit mediaImageChanged();
        return;
    }

    m_nam.clearAccessCache();
    m_nam.clearConnectionCache();

    QNetworkRequest request(imageUrl);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);
    m_nam.get(request);
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
            int newState = Util::convertStringToEnum<MediaPlayerStates::Enum>(uc::Util::FirstToUpper(data.toString()));
            m_state = newState;
            ok = true;
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
                m_mediaDuration = 0;
                emit mediaDurationChanged();

                m_mediaPosition = 0;
                emit mediaPositionChanged();

                m_mediaImageUrl.clear();
                emit mediaImageUrlChanged();
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

            if (!m_mediaImageUrl.contains(newImageUrl) || newImageUrl.isEmpty()) {
                m_mediaImageUrl = newImageUrl;
                ok = true;
                emit mediaImageUrlChanged();

                m_mediaImageDownloadTries = 0;

                getMediaImageColor(m_mediaImageUrl);
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

void MediaPlayer::onNetworkRequestFinished(QNetworkReply *reply) {
    if (reply->error()) {
        qCWarning(lcMediaPlayer()).noquote() << "ERROR" << reply->error();

        if (m_mediaImageDownloadTries > 3) {
            return;
        } else {
            qCDebug(lcMediaPlayer()) << "Image download failed, trying agian" << m_mediaImageUrl;
            QTimer::singleShot(1000, [=] { getMediaImageColor(m_mediaImageUrl); });
            m_mediaImageDownloadTries++;
        }

        reply->deleteLater();
    } else {
        QPixmap p;
        p.loadFromData(reply->readAll());

        int    step = 20;
        int    t = 0;
        int    r = 0, g = 0, b = 0;
        double brightness = 0.6;

        QImage     image = p.toImage();
        QByteArray byteArray;
        QBuffer    buffer(&byteArray);
        image.save(&buffer, "PNG");

        if (!byteArray.isEmpty()) {
            m_mediaImage = QString("data:image/png;base64,");
            m_mediaImage.append(QString::fromLatin1(byteArray.toBase64().data()));
            emit mediaImageChanged();

            for (int i = 0; i < p.width(); i += step) {
                for (int j = 0; j < p.height(); j += step) {
                    if (image.valid(i, j)) {
                        t++;
                        QColor c = image.pixel(i, j);
                        r += c.red();
                        b += c.blue();
                        g += c.green();
                    }
                }
            }

            m_mediaImageColor =
                QColor(static_cast<int>(brightness * r / t) > 255 ? 255 : static_cast<int>(brightness * r / t),
                       static_cast<int>(brightness * g / t) > 255 ? 255 : static_cast<int>(brightness * g / t),
                       static_cast<int>(brightness * b / t) > 255 ? 255 : static_cast<int>(brightness * b / t));
            if (m_mediaImageColor.lightness() < 30) {
                m_mediaImageColor.setHsl(m_mediaImageColor.hslHue(), m_mediaImageColor.hslSaturation(), 30);
            }

            qCDebug(lcMediaPlayer()).noquote() << "Background image lightness" << m_mediaImageColor.lightness();

            emit mediaImageColorChanged();
        }

        reply->deleteLater();
    }
}

}  // namespace entity
}  // namespace ui
}  // namespace uc
