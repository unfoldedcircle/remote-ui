// Copyright (c)  2022-2025 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "voiceAssistant.h"

#include "../../logging.h"
#include "../../util.h"

namespace uc {
namespace ui {
namespace entity {

VoiceAssistant::VoiceAssistant(const QString &id, QVariantMap nameI18n, const QString &language, const QString &icon,
                               const QString &area, const QString &deviceClass, const QStringList &features, bool enabled,
                               QVariantMap attributes, QVariantMap options, const QString &integrationId, QObject *parent)
    : Base(id, nameI18n, language, icon, area, Type::Voice_assistant, enabled, attributes, integrationId, false, parent) {
    qCDebug(lcVoiceAssistant()) << "Voice Assistant entity constructor";

    updateFeatures<VoiceAssistantFeatures::Enum>(features);

    // attributes
    if (attributes.size() > 0) {
        for (QVariantMap::iterator i = attributes.begin(); i != attributes.end(); i++) {
            updateAttribute(uc::Util::FirstToUpper(i.key()), i.value());
        }
    }

    // device class
    int deviceClassEnum = -1;

    if (!deviceClass.isEmpty()) {
        deviceClassEnum = Util::convertStringToEnum<VoiceAssistantDeviceClass::Enum>(deviceClass);
    }

    if (deviceClassEnum != -1) {
        m_deviceClass = deviceClass;
    } else {
        m_deviceClass = QVariant::fromValue(VoiceAssistantDeviceClass::VoiceAssistant).toString();
    }

    if (options.contains("profiles")) {
        updateProfiles(options.value("profiles").toList());
    }
}

VoiceAssistant::~VoiceAssistant() { qCDebug(lcVoiceAssistant()) << "Voice Assistant entity destructor"; }

void VoiceAssistant::voiceStart(int sessionId, bool speechResponse, const QString &profileId, int timeout)
{
    QVariantMap params;

    if (m_audioConfig) {
        QVariantMap audioConfig;
        audioConfig.insert("channels", m_audioConfig->getChannels());
        audioConfig.insert("sample_rate", m_audioConfig->getSampleRate());
        audioConfig.insert("sample_format", m_audioConfig->getSampleFormat());

        params.insert("audio_cfg", audioConfig);
    }

    params.insert("session_id", sessionId);
    params.insert("speech_response", speechResponse);
    params.insert("timeout", timeout == 0 ? 15 : timeout);

    if (!profileId.isEmpty()) {
        params.insert("profile_id", profileId);
    }

    sendCommand(VoiceAssistantCommands::Voice_start, params);
}

void VoiceAssistant::voiceEnd()
{
    sendCommand(VoiceAssistantCommands::Voice_end);
}

QObject *VoiceAssistant::getProfile(const QString &profileId)
{
    return m_profiles.value(profileId);
}

void VoiceAssistant::sendCommand(VoiceAssistantCommands::Enum cmd, QVariantMap params)
{
    Base::sendCommand(QVariant::fromValue(cmd).toString(), params);
}

void VoiceAssistant::sendCommand(VoiceAssistantCommands::Enum cmd) { sendCommand(cmd, QVariantMap()); }

bool VoiceAssistant::updateAttribute(const QString &attribute, QVariant data)
{
    bool ok = false;

            // convert to enum
    VoiceAssistantAttributes::Enum attributeEnum = Util::convertStringToEnum<VoiceAssistantAttributes::Enum>(attribute);

    switch (attributeEnum) {
        case VoiceAssistantAttributes::State: {
            int newState = Util::convertStringToEnum<VoiceAssistantStates::Enum>(uc::Util::FirstToUpper(data.toString()));
            if (newState != -1) {
                m_state = newState;
                ok      = true;

                m_stateAsString = VoiceAssistantStates::getTranslatedString(static_cast<VoiceAssistantStates::Enum>(m_state));
                m_stateInfo = getStateAsString();

                emit stateAsStringChanged();
                emit stateInfoChanged();
                emit stateChanged(m_id, m_state);
            }
            break;
        }
    }

    return ok;
}

bool VoiceAssistant::updateOptions(QVariant data)
{
    bool        ok      = false;
    QVariantMap options = data.toMap();

    // update options
    if (options.contains("profiles")) {
        updateProfiles(options.value("profiles").toList());
        ok         = true;
    }

    return ok;
}

void VoiceAssistant::onLanguageChangedTypeSpecific()
{
    QTimer::singleShot(500, [=]() {
        m_stateAsString = VoiceAssistantStates::getTranslatedString(static_cast<VoiceAssistantStates::Enum>(m_state));
        m_stateInfo = getStateAsString();

        emit stateAsStringChanged();
        emit stateInfoChanged();
        emit stateChanged(m_id, m_state);
    });
}

void VoiceAssistant::updateProfiles(QVariantList data)
{
    qDeleteAll(m_profiles);
    m_profiles.clear();

    for (QVariantList::iterator i = data.begin(); i != data.end(); ++i) {
        QVariantMap p = i->toMap();

        VoiceAssistantProfile* profile = new VoiceAssistantProfile(p.value("id").toString(), p.value("name").toString(), p.value("language").toString(),
                                                                   p.value("features").toList().contains("transcription"),
                                                                   p.value("features").toList().contains("response_text"),
                                                                   p.value("features").toList().contains("response_speech"), this);

        m_profiles.insert(p.value("id").toString(), profile);
    }
}

VoiceAssistantAudioConfig::VoiceAssistantAudioConfig(int channels, int sampleRate, const QString &sampleFormat)
    : m_channels(channels), m_sampleRate(sampleRate), m_sampleFormat(sampleFormat) {}

VoiceAssistantProfile::VoiceAssistantProfile(const QString &id, const QString &name, const QString &language, bool transcription, bool responseText, bool responseSpeech, QObject* parent)
    : QObject(parent), m_id(id), m_name(name), m_language(language), m_transcription(transcription), m_responseText(responseText), m_responseSpeech(responseSpeech) {}

}  // namespace entity
}  // namespace ui
}  // namespace uc
