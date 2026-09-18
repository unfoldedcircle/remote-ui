// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAudioDeviceInfo>
#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QSoundEffect>

#include "../hardware/hardwareModel.h"

namespace uc {
namespace ui {

class SoundEffects : public QObject {
    Q_OBJECT

    Q_PROPERTY(int volume READ getVolume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)

 public:
    explicit SoundEffects(int volume, bool enabled, const QString &effectsDir, hw::HardwareModel::Enum model,
                          QObject *parent = nullptr);
    ~SoundEffects();

    void initialize();

    enum SoundEffect {
        Click,
        ClickLow,
        Confirm,
        Error,
        BatteryCharge,
    };
    Q_ENUM(SoundEffect)

 public:
    // Q_PROPERTY METHODS
    int  getVolume() { return m_volume; }
    void setVolume(int volume);

    bool isEnabled() { return m_enabled; }
    void setEnabled(bool value);

 public:
    // QML accesible methods
    /**
     * @brief play a sound effect
     * @param effect
     */
    Q_INVOKABLE void play(uc::ui::SoundEffects::SoundEffect effect);

 public:
    static QObject *qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine);

    /// True if the sound effect files were found and loaded. False without UC_SOUND_EFFECTS_PATH, e.g. on the desktop.
    bool isAvailable() const { return m_effectClick != nullptr; }

 signals:
    void volumeChanged();
    void enabledChanged();

 private:
    static SoundEffects *s_instance;

    QString       m_effectsDir;
    QSoundEffect *m_effectClick = nullptr;
    QSoundEffect *m_effectClickLow = nullptr;
    QSoundEffect *m_effectConfirm = nullptr;
    QSoundEffect *m_effectError = nullptr;
    QSoundEffect *m_effectBatteryCharge = nullptr;

    hw::HardwareModel::Enum m_model;

    int  m_volume;
    bool m_enabled;

 private:
    void createEffects(const QAudioDeviceInfo &deviceInfo);
};

}  // namespace ui
}  // namespace uc
