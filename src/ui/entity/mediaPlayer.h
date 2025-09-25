// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

/**
 * @see https://github.com/unfoldedcircle/core-api/blob/main/doc/entities/entity_media_player.md
 */

#include <QBuffer>
#include <QColor>
#include <QImage>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPixmap>
#include <QTimer>

#include "entity.h"

namespace uc {
namespace ui {
namespace entity {

class MediaPlayerFeatures : public QObject {
    Q_GADGET

 public:
    enum Enum {
        On_off,
        Toggle,
        Volume,
        Volume_up_down,
        Mute_toggle,
        Mute,
        Unmute,
        Play_pause,
        Stop,
        Next,
        Previous,
        Fast_forward,
        Rewind,
        Repeat,
        Shuffle,
        Seek,
        Media_duration,
        Media_position,
        Media_title,
        Media_artist,
        Media_album,
        Media_image_url,
        Media_type,
        Dpad,
        Numpad,
        Home,
        Menu,
        Context_menu,
        Guide,
        Info,
        Color_buttons,
        Channel_switcher,
        Select_source,
        Select_sound_mode,
        Eject,
        Open_close,
        Audio_track,
        Subtitle,
        Record,
        Settings
    };
    Q_ENUM(Enum)
};

class MediaPlayerAttributes : public QObject {
    Q_GADGET

 public:
    enum Enum {
        State,
        Volume,
        Muted,
        Media_duration,
        Media_position,
        Media_type,
        Media_image_url,
        Media_title,
        Media_artist,
        Media_album,
        Repeat,
        Shuffle,
        Source,
        Source_list,
        Sound_mode,
        Sound_mode_list
    };
    Q_ENUM(Enum)
};

class MediaPlayerStates : public QObject {
    Q_OBJECT

 public:
    enum Enum { Unavailable = 0, Unknown, On, Off, Playing, Paused, Standby, Buffering };
    Q_ENUM(Enum)

    static QString getTranslatedString(Enum state) {
        switch (state) {
            case Enum::Unavailable:
                return QCoreApplication::translate("Media platyer state", "Unavailable");
            case Enum::Unknown:
                return QCoreApplication::translate("Media platyer state", "Unknown");
            case Enum::On:
                return QCoreApplication::translate("Media platyer state", "On");
            case Enum::Off:
                return QCoreApplication::translate("Media platyer state", "Off");
            case Enum::Playing:
                return QCoreApplication::translate("Media platyer state", "Playing");
            case Enum::Paused:
                return QCoreApplication::translate("Media platyer state", "Paused");
            case Enum::Standby:
                return QCoreApplication::translate("Media platyer state", "Standby");
            case Enum::Buffering:
                return QCoreApplication::translate("Media platyer state", "Buffering");
            default:
                return Util::convertEnumToString<Enum>(state);
        }
    }
};

class MediaPlayerCommands : public QObject {
    Q_GADGET

 public:
    enum Enum {
        On,
        Off,
        Toggle,
        Play_pause,
        Stop,
        Previous,
        Next,
        Fast_forward,
        Rewind,
        Seek,
        Volume,
        Volume_up,
        Volume_down,
        Mute_toggle,
        Mute,
        Unmute,
        Repeat,
        Shuffle,
        Channel_up,
        Channel_down,
        Cursor_up,
        Cursor_down,
        Cursor_left,
        Cursor_right,
        Cursor_enter,
        Digit_0,
        Digit_1,
        Digit_2,
        Digit_3,
        Digit_4,
        Digit_5,
        Digit_6,
        Digit_7,
        Digit_8,
        Digit_9,
        Function_red,
        Function_green,
        Function_yellow,
        Function_blue,
        Home,
        Menu,
        Context_menu,
        Guide,
        Info,
        Back,
        Select_source,
        Select_sound_mode,
        Record,
        My_recordings,
        Live,
        Eject,
        Open_close,
        Audio_track,
        Subtitle,
        Settings,
        Search
    };
    Q_ENUM(Enum)
};

class MediaPlayerDeviceClass : public QObject {
    Q_GADGET

 public:
    enum Enum { Receiver, Set_top_box, Speaker, Streaming_box, Tv };
    Q_ENUM(Enum)
};

class MediaPlayerRepeatMode : public QObject {
    Q_GADGET

 public:
    enum Enum { OFF, ALL, ONE };
    Q_ENUM(Enum)
};

class MediaPlayer : public Base {
    Q_OBJECT

    Q_PROPERTY(int volume READ getVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ getMuted NOTIFY mutedChanged)
    Q_PROPERTY(int mediaDuration READ getMediaDuration NOTIFY mediaDurationChanged)
    Q_PROPERTY(int mediaPosition READ getMediaPosition NOTIFY mediaPositionChanged)
    Q_PROPERTY(QString mediaType READ getMediaType NOTIFY mediaTypeChanged)
    Q_PROPERTY(QString mediaImageUrl READ getMediaImageUrl NOTIFY mediaImageUrlChanged)
    Q_PROPERTY(QString mediaImage READ getMediaImage NOTIFY mediaImageChanged)
    Q_PROPERTY(QColor mediaImageColor READ getMediaImageColor NOTIFY mediaImageColorChanged)
    Q_PROPERTY(QString mediaTitle READ getMediaTitle NOTIFY mediaTitleChanged)
    Q_PROPERTY(QString mediaArtist READ getMediaArtist NOTIFY mediaArtistChanged)
    Q_PROPERTY(QString mediaAlbum READ getMediaAlbum NOTIFY mediaAlbumChanged)
    Q_PROPERTY(bool shuffleIsOn READ getShuffle NOTIFY shuffleChanged)
    Q_PROPERTY(int repeatMode READ getRepeat NOTIFY repeatChanged)
    Q_PROPERTY(QString source READ getSource NOTIFY sourceChanged)
    Q_PROPERTY(QStringList sourceList READ getSourceList NOTIFY sourceListChanged)
    //    Q_PROPERTY(TODO soundMode READ getSoundMode NOTIFY soundModeChanged)
    //    Q_PROPERTY(TODO soundModeList READ getSoundModeList NOTIFY soundModeListChanged)

    // options
    Q_PROPERTY(int volumeSteps READ getVolumeSteps CONSTANT)

 public:
    explicit MediaPlayer(const QString &id, QVariantMap nameI18n, const QString &language, const QString &icon,
                         const QString &area, const QString &deviceClass, const QStringList &features, bool enabled,
                         QVariantMap attributes, QVariantMap options, const QString &integrationId, QObject *parent);
    ~MediaPlayer();

    int         getVolume() { return m_volume; }
    bool        getMuted() { return m_muted; }
    int         getMediaDuration() { return m_mediaDuration; }
    int         getMediaPosition() { return m_mediaPosition; }
    QString         getMediaType() { return m_mediaType; }
    QString     getMediaImageUrl() { return m_mediaImageUrl; }
    QString     getMediaImage() { return m_mediaImage; }
    QColor      getMediaImageColor() { return m_mediaImageColor; }
    QString     getMediaTitle() { return m_mediaTitle; }
    QString     getMediaArtist() { return m_mediaArtist; }
    QString     getMediaAlbum() { return m_mediaAlbum; }
    bool        getShuffle() { return m_shuffle; }
    int         getRepeat() { return m_repeat; }
    QString     getSource() { return m_source; }
    QStringList getSourceList() { return m_sourceList; }

    // options
    int getVolumeSteps() { return m_volumeSteps; }

    QString getStateInfo() override { return m_mediaTitle; }

    Q_INVOKABLE void turnOn() override;
    Q_INVOKABLE void turnOff() override;
    Q_INVOKABLE void toggle();
    Q_INVOKABLE void playPause();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void previous();
    Q_INVOKABLE void next();
    Q_INVOKABLE void fastForward();
    Q_INVOKABLE void rewind();
    Q_INVOKABLE void seek(int mediaPosition);
    Q_INVOKABLE void setVolume(int volume);
    Q_INVOKABLE void volumeUp();
    Q_INVOKABLE void volumeDown();
    Q_INVOKABLE void muteToggle();
    Q_INVOKABLE void mute();
    Q_INVOKABLE void unmute();
    Q_INVOKABLE void repeat();
    Q_INVOKABLE void shuffle();
    Q_INVOKABLE void channelUp();
    Q_INVOKABLE void channelDown();
    Q_INVOKABLE void cursorUp();
    Q_INVOKABLE void cursorDown();
    Q_INVOKABLE void cursorLeft();
    Q_INVOKABLE void cursorRight();
    Q_INVOKABLE void cursorEnter();
    Q_INVOKABLE void digit0();
    Q_INVOKABLE void digit1();
    Q_INVOKABLE void digit2();
    Q_INVOKABLE void digit3();
    Q_INVOKABLE void digit4();
    Q_INVOKABLE void digit5();
    Q_INVOKABLE void digit6();
    Q_INVOKABLE void digit7();
    Q_INVOKABLE void digit8();
    Q_INVOKABLE void digit9();
    Q_INVOKABLE void functionRed();
    Q_INVOKABLE void functionGreen();
    Q_INVOKABLE void functionYellow();
    Q_INVOKABLE void functionBlue();
    Q_INVOKABLE void home();
    Q_INVOKABLE void menu();
    Q_INVOKABLE void contextMenu();
    Q_INVOKABLE void guide();
    Q_INVOKABLE void info();
    Q_INVOKABLE void back();
    Q_INVOKABLE void selectSource(const QString &source);
    Q_INVOKABLE void record();
    Q_INVOKABLE void myRecordings();
    Q_INVOKABLE void live();
    Q_INVOKABLE void eject();
    Q_INVOKABLE void openClose();
    Q_INVOKABLE void audioTrack();
    Q_INVOKABLE void subtitle();
    Q_INVOKABLE void settings();
    //    Q_INVOKABLE void selectSoundMode(const QString &soundMode);
    //    Q_INVOKABLE void search(const QString &searchString);

    void sendCommand(MediaPlayerCommands::Enum cmd, QVariantMap params);
    void sendCommand(MediaPlayerCommands::Enum cmd);
    void sendSimpleCommand(QString command);
    bool updateAttribute(const QString &attribute, QVariant data) override;

    void onLanguageChangedTypeSpecific() override;

 signals:
    void volumeChanged();
    void mutedChanged();
    void mediaDurationChanged();
    void mediaPositionChanged();
    void mediaTypeChanged();
    void mediaImageUrlChanged();
    void mediaImageChanged();
    void mediaImageColorChanged();
    void mediaTitleChanged();
    void mediaArtistChanged();
    void mediaAlbumChanged();
    void shuffleChanged();
    void repeatChanged();
    void sourceChanged();
    void sourceListChanged();
    void soundModeChanged();
    void soundModeListChanged();
    void addToActivities(QString entityId);
    void removeFromActivities(QString entityId);

 private:
    int                         m_volume;
    bool                        m_muted;
    int                         m_mediaDuration;
    int                         m_mediaPosition;
    QString                     m_mediaType;
    QString                     m_mediaImageUrl;
    QString                     m_mediaImage;
    QColor                      m_mediaImageColor;
    QString                     m_mediaTitle;
    QString                     m_mediaArtist;
    QString                     m_mediaAlbum;
    bool                        m_shuffle;
    MediaPlayerRepeatMode::Enum m_repeat;
    QString                     m_source;
    QStringList                 m_sourceList;

    // options
    int         m_volumeSteps;
    QStringList m_simpleCommands;

 private:
    QTimer m_positionTimer;

    QNetworkAccessManager m_nam;
    void                  getMediaImageColor(QString imageUrl);
    int                   m_mediaImageDownloadTries = 0;

 private slots:
    void onPositionTimerTimeout();
    void onNetworkRequestFinished(QNetworkReply *reply);
};

}  // namespace entity
}  // namespace ui
}  // namespace uc
