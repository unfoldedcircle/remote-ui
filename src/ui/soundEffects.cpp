// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "soundEffects.h"

#include "../logging.h"

namespace uc {
namespace ui {

SoundEffects *SoundEffects::s_instance = nullptr;

SoundEffects::SoundEffects(int volume, bool enabled, const QString &effectsDir, hw::HardwareModel::Enum model,
                           QObject *parent)
    : QObject(parent), m_effectsDir(effectsDir), m_model(model), m_volume(volume), m_enabled(enabled) {
    Q_ASSERT(s_instance == nullptr);
    s_instance = this;
}

SoundEffects::~SoundEffects() {
    s_instance = nullptr;
}

void SoundEffects::initialize() {
    QAudioDeviceInfo deviceInfo = QAudioDeviceInfo::defaultOutputDevice();
    createEffects(deviceInfo);

    qCDebug(lcUi()) << "Default audio output device:" << deviceInfo.deviceName();
}

void SoundEffects::setVolume(int volume) {
    if (m_volume != volume) {
        m_volume = volume;
        qCDebug(lcUi()) << "Sound effects volume changed to:" << m_volume;
        emit volumeChanged();
    }
}

void SoundEffects::setEnabled(bool value) {
    if (m_enabled != value) {
        m_enabled = value;
        qCDebug(lcUi()) << "Sound effects enabled changed to:" << m_enabled;
        emit enabledChanged();
    }
}

void SoundEffects::play(SoundEffects::SoundEffect effect) {
    if (!m_enabled) {
        return;
    }

    switch (effect) {
        case Click:
            m_effectClick->setVolume(qreal(m_volume) / 100);
            m_effectClick->play();
            break;
        case ClickLow:
            m_effectClickLow->setVolume(qreal(m_volume) / 100);
            m_effectClickLow->play();
            break;
        case Confirm:
            m_effectConfirm->setVolume(qreal(m_volume) / 100);
            m_effectConfirm->play();
            break;
        case Error:
            m_effectError->setVolume(qreal(m_volume) / 100);
            m_effectError->play();
            break;
        case BatteryCharge:
            m_effectBatteryCharge->setVolume(qreal(m_volume) / 100);
            m_effectBatteryCharge->play();
    }
}

QObject *SoundEffects::qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(scriptEngine);

    QObject *obj = s_instance;
    engine->setObjectOwnership(obj, QQmlEngine::CppOwnership);

    return obj;
}

void SoundEffects::createEffects(const QAudioDeviceInfo &deviceInfo) {
    m_effectClick = new QSoundEffect(deviceInfo, this);
    m_effectClick->setSource(QUrl::fromLocalFile(m_effectsDir + QStringLiteral("/click.wav")));

    m_effectClickLow = new QSoundEffect(deviceInfo, this);
    m_effectClickLow->setSource(QUrl::fromLocalFile(m_effectsDir + QStringLiteral("/click_lo.wav")));

    m_effectConfirm = new QSoundEffect(deviceInfo, this);
    m_effectConfirm->setSource(QUrl::fromLocalFile(m_effectsDir + QStringLiteral("/confirm.wav")));

    m_effectError = new QSoundEffect(deviceInfo, this);
    m_effectError->setSource(QUrl::fromLocalFile(m_effectsDir + QStringLiteral("/error.wav")));

    m_effectBatteryCharge = new QSoundEffect(deviceInfo, this);
    m_effectBatteryCharge->setSource(QUrl::fromLocalFile(m_effectsDir + QStringLiteral("/zap_future.wav")));
}

}  // namespace ui
}  // namespace uc
