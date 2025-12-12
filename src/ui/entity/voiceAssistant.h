// Copyright (c) 2022-2025 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

/**
 * @see https://github.com/unfoldedcircle/core-api/blob/main/doc/entities/entity_voice_assistant.md
 */

#include "entity.h"

namespace uc {
namespace ui {
namespace entity {

class VoiceAssistantFeatures : public QObject {
    Q_GADGET
 public:
    enum Enum { Transscription, Response_text, Response_speech };
    Q_ENUM(Enum)
};

class VoiceAssistantAttributes : public QObject {
    Q_GADGET
 public:
    enum Enum { State };
    Q_ENUM(Enum)
};

class VoiceAssistantStates : public QObject {
    Q_OBJECT
 public:
    enum Enum { Unavailable = 0, Unknown, On, Off };
    Q_ENUM(Enum)

    static QString getTranslatedString(Enum state) {
        switch (state) {
            case Enum::Unavailable:
                return QCoreApplication::translate("Voice assistant state", "Unavailable");
            case Enum::Unknown:
                return QCoreApplication::translate("Voice assistant state", "Unknown");
            case Enum::On:
                return QCoreApplication::translate("Voice assistant state", "On");
            case Enum::Off:
                return QCoreApplication::translate("Voice assistant state", "Off");
            default:
                return Util::convertEnumToString<Enum>(state);
        }
    }
};

class VoiceAssistantCommands : public QObject {
    Q_GADGET
 public:
    enum Enum { Voice_start, Voice_end };
    Q_ENUM(Enum)
};

// audio config
class VoiceAssistantAudioConfig : public QObject {
    Q_OBJECT

    Q_PROPERTY(int channels READ getChannels CONSTANT)
    Q_PROPERTY(int sampleRate READ getSampleRate CONSTANT)
    Q_PROPERTY(QString sampleFormat READ getSampleFormat CONSTANT)

 public:
    VoiceAssistantAudioConfig(int channels, int sampleRate, const QString& sampleFormat);
    ~VoiceAssistantAudioConfig() {}

    int getChannels() { return m_channels; }
    int getSampleRate() { return m_sampleRate; }
    QString getSampleFormat() { return m_sampleFormat; }

 private:
    int m_channels;
    int m_sampleRate;
    QString m_sampleFormat;

};

// profile
class VoiceAssistantProfile : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString id READ getId CONSTANT)
    Q_PROPERTY(QString name READ getName CONSTANT)
    Q_PROPERTY(QString language READ getLanguage CONSTANT)
    Q_PROPERTY(bool transcription READ getTranscription CONSTANT)
    Q_PROPERTY(bool responseText READ getResponseText CONSTANT)
    Q_PROPERTY(bool responseSpeech READ getResponseSpeech CONSTANT)

 public:
    VoiceAssistantProfile(const QString& id, const QString& name, const QString& language, bool transcription, bool responseText, bool responseSpeech, QObject *parent);
    ~VoiceAssistantProfile() {}

    QString getId() { return m_id; }
    QString getName() { return m_name; }
    QString getLanguage() { return m_language; }
    bool getTranscription() { return m_transcription; }
    bool getResponseText() { return m_responseText; }
    bool getResponseSpeech() { return m_responseSpeech; }

 private:
    QString m_id;
    QString m_name;
    QString m_language;
    bool m_transcription = false;
    bool m_responseText = false;
    bool m_responseSpeech = false;
};


class VoiceAssistantDeviceClass : public QObject {
    Q_GADGET
 public:
    enum Enum { VoiceAssistant };
    Q_ENUM(Enum)
};

class VoiceAssistant : public Base
{
    Q_OBJECT

    Q_PROPERTY(QObject* audioConfig READ getAudioConfig NOTIFY audioConfigChanged)
    Q_PROPERTY(QString preferredProfile READ getPreferredProfile NOTIFY preferredProfileChanged)

 public:
    explicit VoiceAssistant(const QString &id, QVariantMap nameI18n, const QString &language, const QString &icon,
                    const QString &area, const QString &deviceClass, const QStringList &features, bool enabled,
                    QVariantMap attributes, QVariantMap options, const QString &integrationId, QObject *parent);
    ~VoiceAssistant();

    QObject* getAudioConfig() { return m_audioConfig; }
    QString getPreferredProfile() { return m_preferredProfile; }

    Q_INVOKABLE void voiceStart(int sessionId, bool speechResponse, const QString& profileId, int timeout = 0);
    Q_INVOKABLE void voiceEnd();

    Q_INVOKABLE QObject* getProfile(const QString& profileId);

    void sendCommand(VoiceAssistantCommands::Enum cmd, QVariantMap params);
    void sendCommand(VoiceAssistantCommands::Enum cmd);
    bool updateAttribute(const QString &attribute, QVariant data) override;
    bool updateOptions(QVariant data) override;

    void onLanguageChangedTypeSpecific() override;

 signals:
    void audioConfigChanged();
    void preferredProfileChanged();

 private:
    VoiceAssistantAudioConfig* m_audioConfig = nullptr;
    QHash<QString, VoiceAssistantProfile*> m_profiles;
    QString m_preferredProfile;

    void updateProfiles(QVariantList data);
};


}  // namespace entity
}  // namespace ui
}  // namespace uc

